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
#include "RoomUtils.h"
#include "DataChannel.h"
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
    : Bricks::LoggableS<TransportListener>(logger)
    , _listener(listener)
    , _joinResponse(joinResponse)
    , _publisher(SignalTarget::Publisher, this, pcf, conf, logger)
    , _subscriber(SignalTarget::Subscriber, this, pcf, conf, logger)
    , _pingPongKit(listener, positiveOrZero(joinResponse._pingInterval), positiveOrZero(joinResponse._pingTimeout), pcf)
    , _state(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
{
    _publisher.createDataChannel(DataChannel::lossyDCLabel(), {.ordered = true, .maxRetransmits = 0});
    _publisher.createDataChannel(DataChannel::reliableDCLabel(), {.ordered = true});
}

TransportManager::~TransportManager()
{
    _subscriber.close();
    _publisher.close();
    stopPing();
}

bool TransportManager::valid() const noexcept
{
    return _subscriber.valid() && _publisher.valid();
}

webrtc::PeerConnectionInterface::PeerConnectionState TransportManager::state() const noexcept
{
    return _state;
}

bool TransportManager::closed() const noexcept
{
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed == state();
}

void TransportManager::negotiate(bool force)
{
    // if publish only, negotiate
    if (force || canNegotiate()) {
        createPublisherOffer();
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

bool TransportManager::addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    return _publisher.addTrack(std::move(track));
}

bool TransportManager::removeTrack(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    return _publisher.removeTrack(std::move(sender));
}

bool TransportManager::removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    return _publisher.removeTrack(track);
}

bool TransportManager::addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    switch (target) {
        case SignalTarget::Publisher:
            return _publisher.addIceCandidate(std::move(candidate));
            break;
        case SignalTarget::Subscriber:
            return _subscriber.addIceCandidate(std::move(candidate));
            break;
    }
    return false;
}

void TransportManager::close()
{
    _publisher.close();
    _subscriber.close();
    stopPing();
    updateState();
}

void TransportManager::createPublisherOffer()
{
    if (_publisher) {
        if (localDataChannelsAreCreated()) {
            _pendingNegotiation = false;
            _publisher.createOffer();
        }
        else {
            logVerbose("publisher's offer creation is postponed until all DCs aren't created");
            // wait until DC are not created, and make offer ASAP in [onDataChannel] handler
            _pendingNegotiation = true;
        }
    }
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

bool TransportManager::isPrimary(SignalTarget target) const noexcept
{
    return target == primaryTransport().target();;
}

void TransportManager::updateState()
{
    const auto newState = primaryTransport().state();
    const auto oldState = _state.exchange(newState);
    if (oldState != newState) {
        logInfo(makeStateChangesString(oldState, newState));
        if (webrtc::PeerConnectionInterface::PeerConnectionState::kClosed == newState) {
            _embeddedDCCount = 0U;
        }
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

void TransportManager::onSdpCreated(SignalTarget target,
                                    std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    switch (target) {
        case SignalTarget::Publisher:
            _publisher.setLocalDescription(std::move(desc));
            break;
        case SignalTarget::Subscriber:
            _subscriber.setLocalDescription(std::move(desc));
            break;
    }
}

void TransportManager::onSdpCreationFailure(SignalTarget target, webrtc::SdpType type,
                                            webrtc::RTCError error)
{
    if (canLogError()) {
        logError("Failed to create of " + sdpTypeToString(type) +
                 std::string(" SDP for ") + toString(target) +
                 ": " + error.message());
    }
}

void TransportManager::onSdpSet(SignalTarget target, bool local,
                                const webrtc::SessionDescriptionInterface* desc)
{
    if (local) {
        // offer is always from publisher (see also [negotiate])
        // answer from subscriber
        switch (target) {
            case SignalTarget::Publisher:
                invoke(&TransportManagerListener::onPublisherOffer, desc);
                break;
            case SignalTarget::Subscriber:
                invoke(&TransportManagerListener::onSubscriberAnswer, desc);
                break;
        }
    }
    else { // remote
        switch (target) {
            case SignalTarget::Publisher: // see [setRemoteAnswer]
                if (!canNegotiate()) {
                    createPublisherOffer();
                }
                break;
            case SignalTarget::Subscriber: // see [setRemoteOffer]
                _subscriber.createAnswer();
                break;
                
        }
    }
}

void TransportManager::onSdpSetFailure(SignalTarget target, bool local, webrtc::RTCError error)
{
    if (canLogError()) {
        logError("Failed to set of " + std::string(local ? "local SDP for " : "remote SDP for ") +
                 toString(target) + ": " +
                 error.message());
    }
}

void TransportManager::onLocalTrackAdded(SignalTarget target,
                                         rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (SignalTarget::Publisher == target) {
        invoke(&TransportManagerListener::onLocalTrackAdded, std::move(sender));
    }
}

void TransportManager::onLocalTrackAddFailure(SignalTarget target,
                                              const std::string& id,
                                              cricket::MediaType type,
                                              const std::vector<std::string>& streamIds,
                                              webrtc::RTCError error)
{
    if (SignalTarget::Publisher == target) {
        invoke(&TransportManagerListener::onLocalTrackAddFailure, id, type,
               streamIds, std::move(error));
    }
}

void TransportManager::onLocalTrackRemoved(SignalTarget target,
                                           const std::string& id,
                                           cricket::MediaType type,
                                           const std::vector<std::string>&)
{
    if (SignalTarget::Publisher == target) {
        invoke(&TransportManagerListener::onLocalTrackRemoved, id, type);
    }
}

void TransportManager::onLocalDataChannelCreated(SignalTarget target,
                                                 rtc::scoped_refptr<DataChannel> channel)
{
    if (SignalTarget::Publisher == target && channel) {
        const auto label = channel->label();
        if (DataChannel::lossyDCLabel() == label || DataChannel::reliableDCLabel() == label) {
            const auto embeddedDCCount = 1U + _embeddedDCCount.fetch_add(1U);
            if (localDataChannelsAreCreated() && _pendingNegotiation.exchange(false)) {
                logVerbose("all DCs have been create, we have a pending offer - let's try to create it");
                createPublisherOffer();
            }
        }
        invoke(&TransportManagerListener::onLocalDataChannelCreated, std::move(channel));
    }
}

void TransportManager::onConnectionChange(SignalTarget target,
                                          webrtc::PeerConnectionInterface::PeerConnectionState)
{
    if (isPrimary(target)) {
        updateState();
    }
}

void TransportManager::onIceConnectionChange(SignalTarget target,
                                             webrtc::PeerConnectionInterface::IceConnectionState)
{
    if (isPrimary(target)) {
        updateState();
    }
}

void TransportManager::onSignalingChange(SignalTarget target,
                                         webrtc::PeerConnectionInterface::SignalingState)
{
    if (isPrimary(target)) {
        updateState();
    }
}

void TransportManager::onNegotiationNeededEvent(SignalTarget target, uint32_t /*eventId*/)
{
    if (SignalTarget::Publisher == target && localDataChannelsAreCreated()) {
        invoke(&TransportManagerListener::onNegotiationNeeded);
    }
}

void TransportManager::onRemoteDataChannelOpened(SignalTarget target,
                                                 rtc::scoped_refptr<DataChannel> channel)
{
    if (SignalTarget::Subscriber == target) {
        // in subscriber primary mode, server side opens sub data channels
        invoke(&TransportManagerListener::onRemoteDataChannelOpened, std::move(channel));
    }
}

void TransportManager::onIceCandidateGathered(SignalTarget target,
                                              const webrtc::IceCandidateInterface* candidate)
{
    if (candidate) {
        invoke(&TransportManagerListener::onIceCandidateGathered, target, candidate);
    }
}

void TransportManager::onRemoteTrackAdded(SignalTarget target,
                                          rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Subscriber == target) {
        invoke(&TransportManagerListener::onRemoteTrackAdded, std::move(transceiver));
    }
}

void TransportManager::onRemotedTrackRemoved(SignalTarget target,
                                             rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (SignalTarget::Subscriber == target) {
        invoke(&TransportManagerListener::onRemotedTrackRemoved, std::move(receiver));
    }
}

std::string_view TransportManager::logCategory() const
{
    static const std::string_view category("transport_manager");
    return category;
}

} // namespace LiveKitCpp
