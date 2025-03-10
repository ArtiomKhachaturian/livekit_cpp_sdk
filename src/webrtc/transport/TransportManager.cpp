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

namespace {

template<typename T>
inline uint32_t positiveOrZero(T value) {
    return value > 0 ? static_cast<uint32_t>(value) : 0U;
}

}

namespace LiveKitCpp
{
TransportManager::TransportManager(const JoinResponse& joinResponse,
                                   TransportManagerListener* listener,
                                   const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                   const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TransportListener, DataChannelListener>(logger)
    , _listener(listener)
    , _joinResponse(joinResponse)
    , _publisher(SignalTarget::Publisher, this, pcf, conf, logger)
    , _subscriber(SignalTarget::Subscriber, this, pcf, conf, logger)
    , _pingPongKit(listener, positiveOrZero(joinResponse._pingInterval), positiveOrZero(joinResponse._pingTimeout), pcf)
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

void TransportManager::negotiate(bool startPing)
{
    // if publish only, negotiate
    if (canNegotiate()) {
        _publisher.createOffer();
    }
    if (startPing) {
        this->startPing();
    }
}

bool TransportManager::setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (desc) {
        if (webrtc::SdpType::kOffer == desc->GetType()) {
            _subscriber.setRemoteDescription(std::move(desc));
            return true;
        }
        if (canLogError()) {
            logError("Incorrect remote SDP type for " + toString(_subscriber.target()) +
                     " , actual is '" + desc->type() + "', but '" +
                     sdpTypeToString(webrtc::SdpType::kOffer) + "'is expected");
        }
    }
    return false;
}

bool TransportManager::setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
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
            logError("Incorrect remote SDP type for " +
                     toString(_publisher.target()) + ", actual is '" +
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

void TransportManager::addRemoteIceCandidate(SignalTarget target,
                                             std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    switch (target) {
        case SignalTarget::Publisher:
            _publisher.addRemoteIceCandidate(std::move(candidate));
            break;
        case SignalTarget::Subscriber:
            _subscriber.addRemoteIceCandidate(std::move(candidate));
            break;
    }
}

void TransportManager::close()
{
    _publisher.close();
    _subscriber.close();
    stopPing();
    updateState();
}

bool TransportManager::canNegotiate() const noexcept
{
    return !_joinResponse._subscriberPrimary || _joinResponse._fastPublish;
}

Transport& TransportManager::primaryTransport() noexcept
{
    return _joinResponse._subscriberPrimary ? _subscriber : _publisher;
}

const Transport& TransportManager::primaryTransport() const noexcept
{
    return _joinResponse._subscriberPrimary ? _subscriber : _publisher;
}

void TransportManager::updateState()
{
    const auto newState = primaryTransport().state();
    const auto oldState = _state.exchange(newState);
    if (oldState != newState) {
        logInfo(makeStateChangesString(oldState, newState));
        invoke(&TransportManagerListener::onStateChange, newState,
               _publisher.state(), _subscriber.state());
    }
}


template <class Method, typename... Args>
void TransportManager::invoke(const Method& method, Args&&... args) const
{
    if (_listener) {
        ((*_listener).*method)(std::forward<Args>(args)...);
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
    if (local) {
        // offer is always from publisher (see also [negotiate])
        // answer from subscriber
        switch (transport.target()) {
            case SignalTarget::Publisher:
                invoke(&TransportManagerListener::onPublisherOffer, desc);
                break;
            case SignalTarget::Subscriber:
                invoke(&TransportManagerListener::onSubscriberAnswer, desc);
                break;
        }
    }
    else { // remote
        switch (transport.target()) {
            case SignalTarget::Publisher: // see [setRemoteAnswer]
                if (!canNegotiate()) {
                    transport.createOffer();
                }
                break;
            case SignalTarget::Subscriber: // see [setRemoteOffer]
                transport.createAnswer();
                break;
                
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
        logError("Failed to set configuration for " + toString(transport.target()) +
                 ": " + error.message());
    }
}

void TransportManager::onConnectionChange(Transport& transport, webrtc::PeerConnectionInterface::PeerConnectionState)
{
    if (&transport == &primaryTransport()) {
        updateState();
    }
}

void TransportManager::onIceConnectionChange(Transport& transport, webrtc::PeerConnectionInterface::IceConnectionState)
{
    if (&transport == &primaryTransport()) {
        updateState();
    }
}

void TransportManager::onSignalingChange(Transport& transport, webrtc::PeerConnectionInterface::SignalingState)
{
    if (&transport == &primaryTransport()) {
        updateState();
    }
}

void TransportManager::onDataChannel(Transport& transport, rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    if (SignalTarget::Subscriber == transport.target()) {
        // in subscriber primary mode, server side opens sub data channels
        invoke(&TransportManagerListener::onDataChannel, std::move(channel));
    }
}

void TransportManager::onIceCandidate(Transport& transport, const webrtc::IceCandidateInterface* candidate)
{
    if (candidate) {
        invoke(&TransportManagerListener::onIceCandidate, transport.target(), candidate);
    }
}

void TransportManager::onTrack(Transport& transport, rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Subscriber == transport.target()) {
        invoke(&TransportManagerListener::onRemoteTrack, std::move(transceiver));
    }
}

void TransportManager::onStateChange(DataChannelType channelType)
{
    if (canLogInfo()) {
        logInfo("the data channel '" + toString(channelType) + "' state have changed");
    }
}

void TransportManager::onMessage(DataChannelType channelType,
                                 const webrtc::DataBuffer& /*buffer*/)
{
    if (canLogInfo()) {
        logInfo("a message buffer was successfully received for '" +
                toString(channelType) + " data channel");
    }
}

void TransportManager::onBufferedAmountChange(DataChannelType /*channelType*/,
                                              uint64_t /*sentDataSize*/)
{
    
}

std::string_view TransportManager::logCategory() const
{
    static const std::string_view category("transport_manager");
    return category;
}

} // namespace LiveKitCpp
