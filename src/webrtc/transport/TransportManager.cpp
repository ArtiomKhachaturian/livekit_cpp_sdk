// Copyright 2025 Artiom Khachaturian
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "TransportManager.h"
#include "TransportManagerListener.h"
#include "PeerConnectionFactory.h"
#include "DataChannelObserver.h"
#include "DataChannelType.h"
#include "RoomUtils.h"
#include "Utils.h"
#include <list>

namespace {

template<typename T>
inline bool every(const T& required, const std::list<T>& vals) {
    if (!vals.empty()) {
        for (const auto& val : vals) {
            if (val != required) {
                return false;
            }
        }
        return true;
    }
    return false;
}

}

namespace LiveKitCpp
{
TransportManager::TransportManager(bool subscriberPrimary,
                                   TransportManagerListener* listener,
                                   const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                   const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TransportListener, DataChannelListener>(logger)
    , _listener(listener)
    , _subscriberPrimary(subscriberPrimary)
    , _publisher(SignalTarget::Publisher, this, pcf, conf, logger)
    , _subscriber(SignalTarget::Subscriber, this, pcf, conf, logger)
    , _state(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
{
    if (_publisher) {
        DataChannelListener* const listener = this;
        auto observer = std::make_unique<DataChannelObserver>(DataChannelType::Lossy, listener);
        _lossyDC = _publisher.createDataChannel(toString(DataChannelType::Lossy),
                                                {.ordered = true, .maxRetransmits = 0},
                                                observer.get());
        if (_lossyDC) {
            _lossyDCObserver = std::move(observer);
            observer = std::make_unique<DataChannelObserver>(DataChannelType::Relaible, listener);
            _reliableDC = _publisher.createDataChannel(toString(DataChannelType::Relaible),
                                                    {.ordered = true}, observer.get());
            if (_reliableDC) {
                _reliableDCObserver = std::move(observer);
            }
            else {
                // TODO: log error
            }
        }
        else {
            // TODO: log error
        }
    }
}

TransportManager::~TransportManager()
{
    _subscriber.close();
    _publisher.close();
    if (_lossyDC) {
        _lossyDC->UnregisterObserver();
    }
    if (_reliableDC) {
        _reliableDC->UnregisterObserver();
    }
}

bool TransportManager::valid() const noexcept
{
    return _subscriber.valid() && _publisher.valid();
}

webrtc::PeerConnectionInterface::PeerConnectionState TransportManager::state() const noexcept
{
    return _state;
}

/*bool TransportManager::addIceCandidate(const webrtc::IceCandidateInterface* candidate,
                                       SignalTarget target)
{
    if (candidate) {
        switch (target) {
            case SignalTarget::Publisher:
                return _publisher.addIceCandidate(candidate);
            case SignalTarget::Subscriber:
                return _subscriber.addIceCandidate(candidate);
            default:
                break;
        }
    }
    return false;
}*/

void TransportManager::createPublisherOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    _publisher.createOffer(options);
}

void TransportManager::createSubscriberAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options)
{
    _subscriber.createAnswer(options);
}

bool TransportManager::setSubscriberRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        const auto type = desc->GetType();
        if (webrtc::SdpType::kOffer == type) {
            _subscriber.setRemoteDescription(std::move(desc));
            return true;
        }
        if (canLogError()) {
            logError("Incorrect remote SDP type for subscriber, actual is '" +
                     sdpTypeToString(type) + "', but '" +
                     sdpTypeToString(webrtc::SdpType::kOffer) + "'is expected");
        }
    }
    return false;
}

bool TransportManager::setPublisherRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        const auto type = desc->GetType();
        switch (type) {
            case webrtc::SdpType::kPrAnswer:
            case webrtc::SdpType::kAnswer:
                _publisher.setRemoteDescription(std::move(desc));
                return true;
            default:
                break;
        }
        if (canLogError()) {
            logError("Incorrect remote SDP type for publisher, actual is '" +
                     sdpTypeToString(type) + "', but '" +
                     sdpTypeToString(webrtc::SdpType::kAnswer) + "' or '" +
                     sdpTypeToString(webrtc::SdpType::kPrAnswer) + "' are expected");
        }
    }
    return false;
}

bool TransportManager::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    return _subscriber.setConfiguration(config) && _publisher.setConfiguration(config);
}

rtc::scoped_refptr<webrtc::RtpSenderInterface> TransportManager::
    addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    return _publisher.addTrack(std::move(track));
}

void TransportManager::close()
{
    if (webrtc::PeerConnectionInterface::SignalingState::kClosed != _publisher.signalingState()) {
        for (auto&& sender : _publisher.senders()) {
            _publisher.removeTrack(sender);
        }
    }
    _publisher.close();
    _subscriber.close();
    updateState();
}

void TransportManager::updateState()
{
    auto newState = state();
    const std::list<webrtc::PeerConnectionInterface::PeerConnectionState> states = {
        _subscriber.state(), _publisher.state()
    };
    if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kConnected, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kConnected;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kFailed, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kFailed;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kClosed, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kClosed;
    }
    else if (every(webrtc::PeerConnectionInterface::PeerConnectionState::kNew, states)) {
        newState = webrtc::PeerConnectionInterface::PeerConnectionState::kNew;
    }
    const auto oldState = _state.exchange(newState);
    if (oldState != newState) {
        if (canLogInfo()) {
            logInfo(makeStateChangesString(oldState, newState));
        }
        if (_listener) {
            _listener->onStateChange(newState, _publisher.state(), _subscriber.state());
        }
    }
}

void TransportManager::onSdpCreated(Transport& transport,
                                    std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    transport.setLocalDescription(std::move(desc));
}

void TransportManager::onSdpCreationFailure(Transport& transport, webrtc::SdpType type,
                                            webrtc::RTCError error)
{
    if (canLogError()) {
        logError("Failed to create of " + sdpTypeToString(type) +
                 std::string(" SDP for ") + toString(transport.target()) +
                 ": " + error.message());
    }
}

void TransportManager::onSdpSet(Transport& transport, bool local,
                                const webrtc::SessionDescriptionInterface* desc)
{
    if (SignalTarget::Publisher == transport.target()) {
        if (local && _listener) {
            _listener->onPublisherOffer(desc);
        }
    }
    else { // subscriber
        if (local) {
            if (_listener) {
                _listener->onSubscriberAnswer(desc);
            }
        }
        else {
            _subscriber.createAnswer();
        }
    }
}

void TransportManager::onSdpSetFailure(Transport& transport, bool local, webrtc::RTCError error)
{
    if (canLogError()) {
        logError("Failed to set of " + std::string(local ? "local SDP for " : "remote SDP for ") +
                 toString(transport.target()) + ": " +
                 error.message());
    }
}

void TransportManager::onSetConfigurationError(Transport& transport, webrtc::RTCError error)
{
    if (canLogError()) {
        logError("Failed to set configuration for " +
                 std::string(toString(transport.target())) +
                 ": " + error.message());
    }
}

void TransportManager::onConnectionChange(Transport&, webrtc::PeerConnectionInterface::PeerConnectionState)
{
    updateState();
}

void TransportManager::onIceConnectionChange(Transport&, webrtc::PeerConnectionInterface::IceConnectionState)
{
    updateState();
}

void TransportManager::onSignalingChange(Transport&, webrtc::PeerConnectionInterface::SignalingState)
{
    updateState();
}

void TransportManager::onDataChannel(Transport& transport, rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    if (_listener && SignalTarget::Subscriber == transport.target()) {
        // in subscriber primary mode, server side opens sub data channels
        _listener->onDataChannel(std::move(channel));
    }
}

void TransportManager::onIceCandidate(Transport& transport, const webrtc::IceCandidateInterface* candidate)
{
    if (candidate && _listener) {
        _listener->onIceCandidate(transport.target(), candidate);
    }
}

void TransportManager::onTrack(Transport& transport, rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (_listener && SignalTarget::Subscriber == transport.target()) {
        _listener->onRemoteTrack(std::move(transceiver));
    }
}

void TransportManager::onStateChange(DataChannelType channelType)
{
    if (canLogInfo()) {
        logInfo("the data channel '" + std::string(toString(channelType)) + "' state have changed");
    }
}

void TransportManager::onMessage(DataChannelType channelType,
                                 const webrtc::DataBuffer& /*buffer*/)
{
    if (canLogInfo()) {
        logInfo("a message buffer was successfully received for '" +
                std::string(toString(channelType)) + " data channel");
    }
}

void TransportManager::onBufferedAmountChange(DataChannelType /*channelType*/,
                                              uint64_t /*sentDataSize*/)
{
    
}

std::string_view TransportManager::logCategory() const
{
    static const std::string_view category("TransportManager");
    return category;
}

} // namespace LiveKitCpp
