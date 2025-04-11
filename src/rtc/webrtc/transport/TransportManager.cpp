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

template <typename T>
inline uint32_t positiveOrZero(T value) {
    return value > 0 ? static_cast<uint32_t>(value) : 0U;
}

}

namespace LiveKitCpp
{

TransportManager::TransportManager(bool subscriberPrimary, bool fastPublish,
                                   int32_t pingTimeout, int32_t pingInterval,
                                   uint64_t negotiationDelay,
                                   const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                   const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                                   const std::string& identity,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TransportListener, PingPongKitListener>(logger)
    , _negotiationTimerId(reinterpret_cast<uint64_t>(this))
    , _negotiationDelay(negotiationDelay)
    , _subscriberPrimary(subscriberPrimary)
    , _fastPublish(fastPublish)
    , _logCategory("transport_manager_" + identity)
    , _negotiationTimer(_negotiationDelay ? new MediaTimer(pcf) : nullptr)
    , _publisher(SignalTarget::Publisher, this, pcf, conf, identity, logger)
    , _subscriber(SignalTarget::Subscriber, this, pcf, conf, identity, logger)
    , _pingPongKit(this, positiveOrZero(pingInterval), positiveOrZero(pingTimeout), pcf)
    , _state(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
{
}

TransportManager::~TransportManager()
{
    close();
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
    if (track) {
        webrtc::RtpTransceiverInit init;
        init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
        return _publisher.addTransceiver(std::move(track), init);
    }
    return false;
}

bool TransportManager::removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    return _publisher.removeTrack(track);
}

void TransportManager::addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate)
{
    switch (target) {
        case SignalTarget::Publisher:
            _publisher.addIceCandidate(std::move(candidate));
            break;
        case SignalTarget::Subscriber:
            _subscriber.addIceCandidate(std::move(candidate));
            break;
    }
}

void TransportManager::queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (callback && !closed()) {
        _publisher.querySenderStats(callback);
        _subscriber.queryReceiverStats(callback);
    }
}

void TransportManager::queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                  const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (receiver && callback && !closed()) {
        _subscriber.queryReceiverStats(callback, receiver);
    }
}

void TransportManager::queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                                  const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (sender && callback && !closed()) {
        _publisher.querySenderStats(callback, sender);
    }
}

void TransportManager::setAudioPlayout(bool playout)
{
    _publisher.setAudioPlayout(playout);
    _subscriber.setAudioPlayout(playout);
}

void TransportManager::setAudioRecording(bool recording)
{
    _publisher.setAudioRecording(recording);
    _subscriber.setAudioRecording(recording);
}

void TransportManager::close()
{
    _publisher.close();
    _subscriber.close();
    stopPing();
    updateState();
}

void TransportManager::setListener(TransportManagerListener* listener)
{
    if (listener) {
        _listener = listener;
        if (!_embeddedDCRequested.exchange(true)) {
            webrtc::DataChannelInit lossy, reliable;
            lossy.ordered = reliable.ordered = true;
            lossy.maxRetransmits = 0;
            _publisher.createDataChannel(DataChannel::lossyLabel(), lossy);
            _publisher.createDataChannel(DataChannel::reliableLabel(), reliable);
        }
    }
    else {
        _listener.reset();
    }
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
    return !_subscriberPrimary || _fastPublish;
}

Transport& TransportManager::primaryTransport() noexcept
{
    return _subscriberPrimary ? _subscriber : _publisher;
}

const Transport& TransportManager::primaryTransport() const noexcept
{
    return _subscriberPrimary ? _subscriber : _publisher;
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
        switch (newState) {
            case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
                _embeddedDCCount = 0U;
            case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
                if (_negotiationTimer) {
                    _negotiationTimer->cancelSingleShot(_negotiationTimerId);
                }
                break;
            default:
                break;
        }
        _listener.invoke(&TransportManagerListener::onStateChange, newState,
                         _publisher.state(), _subscriber.state());
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
    _listener.invoke(&TransportManagerListener::onSdpOperationFailed,
                     target, std::move(error));
}

void TransportManager::onSdpSet(SignalTarget target, bool local,
                                const webrtc::SessionDescriptionInterface* desc)
{
    if (local) {
        // offer is always from publisher (see also [negotiate])
        // answer from subscriber
        switch (target) {
            case SignalTarget::Publisher:
                _listener.invoke(&TransportManagerListener::onPublisherOffer, desc);
                break;
            case SignalTarget::Subscriber:
                _listener.invoke(&TransportManagerListener::onSubscriberAnswer, desc);
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
    _listener.invoke(&TransportManagerListener::onSdpOperationFailed, target, std::move(error));
}

void TransportManager::onTransceiverAdded(SignalTarget target,
                                          rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Publisher == target && transceiver) {
        _listener.invoke(&TransportManagerListener::onLocalTrackAdded, transceiver->sender());
    }
}

void TransportManager::onTransceiverAddFailure(SignalTarget target,
                                               const std::string& id,
                                               cricket::MediaType type,
                                               const webrtc::RtpTransceiverInit& init,
                                               webrtc::RTCError error)
{
    if (SignalTarget::Publisher == target) {
        _listener.invoke(&TransportManagerListener::onLocalTrackAddFailure, id, type,
                         init.stream_ids, std::move(error));
    }
}

void TransportManager::onLocalTrackRemoved(SignalTarget target,
                                           const std::string& id,
                                           cricket::MediaType type,
                                           const std::vector<std::string>&)
{
    if (SignalTarget::Publisher == target) {
        _listener.invoke(&TransportManagerListener::onLocalTrackRemoved, id, type);
    }
}

void TransportManager::onLocalDataChannelCreated(SignalTarget target,
                                                 rtc::scoped_refptr<DataChannel> channel)
{
    if (SignalTarget::Publisher == target && channel) {
        const auto label = channel->label();
        if (DataChannel::lossyLabel() == label || DataChannel::reliableLabel() == label) {
            const auto embeddedDCCount = 1U + _embeddedDCCount.fetch_add(1U);
            if (localDataChannelsAreCreated() && _pendingNegotiation.exchange(false)) {
                logVerbose("all DCs have been create, we have a pending offer - let's try to create it");
                createPublisherOffer();
            }
        }
        _listener.invoke(&TransportManagerListener::onLocalDataChannelCreated, std::move(channel));
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
    if (SignalTarget::Publisher == target && localDataChannelsAreCreated() && !closed()) {
        if (_negotiationTimer) {
            _negotiationTimer->cancelSingleShot(_negotiationTimerId);
            _negotiationTimer->singleShot([this](){
                _listener.invoke(&TransportManagerListener::onNegotiationNeeded);
            }, _negotiationDelay, _negotiationTimerId);
        }
        else {
            _listener.invoke(&TransportManagerListener::onNegotiationNeeded);
        }
    }
}

void TransportManager::onRemoteDataChannelOpened(SignalTarget target,
                                                 rtc::scoped_refptr<DataChannel> channel)
{
    if (SignalTarget::Subscriber == target) {
        // in subscriber primary mode, server side opens sub data channels
        _listener.invoke(&TransportManagerListener::onRemoteDataChannelOpened, std::move(channel));
    }
}

void TransportManager::onIceCandidateGathered(SignalTarget target,
                                              const webrtc::IceCandidateInterface* candidate)
{
    if (candidate) {
        _listener.invoke(&TransportManagerListener::onIceCandidateGathered, target, candidate);
    }
}

void TransportManager::onRemoteTrackAdded(SignalTarget target,
                                          rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Subscriber == target) {
        _listener.invoke(&TransportManagerListener::onRemoteTrackAdded, std::move(transceiver));
    }
}

void TransportManager::onRemotedTrackRemoved(SignalTarget target,
                                             rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (SignalTarget::Subscriber == target) {
        _listener.invoke(&TransportManagerListener::onRemotedTrackRemoved, std::move(receiver));
    }
}

bool TransportManager::onPingRequested()
{
    return _listener.invokeR<bool>(&PingPongKitListener::onPingRequested);
}

void TransportManager::onPongTimeout()
{
    _listener.invoke(&PingPongKitListener::onPongTimeout);
}

} // namespace LiveKitCpp
