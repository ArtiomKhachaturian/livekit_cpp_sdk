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
#include "TransportManagerImpl.h"
#include "TransportManagerListener.h"
#include "PeerConnectionFactory.h"
#include "DataChannel.h"
#include "RoomUtils.h"
#include "RtcUtils.h"
#include "SdpPatch.h"
#include "Utils.h"

namespace {

using namespace LiveKitCpp;

template <typename T>
inline uint32_t positiveOrZero(T value) {
    return value > 0 ? static_cast<uint32_t>(value) : 0U;
}

// https://github.com/livekit/client-sdk-js/blob/main/src/room/utils.ts#L33
inline std::pair<std::string, std::string> unpackStreamId(std::string streamId) {
    if (!streamId.empty()) {
        auto parts = split(streamId, "|");
        if (parts.size() > 1U) {
            if (2U == parts.size()) {
                return {std::move(parts[0U]), std::move(parts[1U])};
            }
            const auto p1s = parts[0].size();
            return {std::move(parts[0]), streamId.substr(p1s + 1)};
        }
    }
    return {streamId, std::string{}};
}

}

namespace LiveKitCpp
{

TransportManagerImpl::TransportManagerImpl(bool subscriberPrimary, bool fastPublish,
                                           int32_t pingTimeout, int32_t pingInterval,
                                           uint64_t negotiationDelay,
                                           const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                           const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                                           const std::string& identity,
                                           const std::string& prefferedAudioEncoder,
                                           const std::string& prefferedVideoEncoder,
                                           const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TransportListener, PingPongKitListener>(logger)
    , _negotiationTimerId(reinterpret_cast<uint64_t>(this))
    , _listener(pcf)
    , _negotiationDelay(negotiationDelay)
    , _subscriberPrimary(subscriberPrimary)
    , _fastPublish(fastPublish)
    , _prefferedAudioEncoder(prefferedAudioEncoder)
    , _prefferedVideoEncoder(prefferedVideoEncoder)
    , _logCategory("transport_manager_" + identity)
    , _negotiationTimer(_negotiationDelay ? new MediaTimer(pcf) : nullptr)
    , _publisher(SignalTarget::Publisher, this, pcf, conf, identity, logger)
    , _subscriber(SignalTarget::Subscriber, this, pcf, conf, identity, logger)
    , _pingPongKit(positiveOrZero(pingInterval), positiveOrZero(pingTimeout), pcf)
    , _state(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
{
}

TransportManagerImpl::~TransportManagerImpl()
{
    close();
}

bool TransportManagerImpl::valid() const noexcept
{
    return _subscriber.valid() && _publisher.valid();
}

webrtc::PeerConnectionInterface::PeerConnectionState TransportManagerImpl::state() const noexcept
{
    return _state;
}

bool TransportManagerImpl::closed() const noexcept
{
    return webrtc::PeerConnectionInterface::PeerConnectionState::kClosed == state();
}

void TransportManagerImpl::negotiate(bool force)
{
    // if publish only, negotiate
    if (force || canNegotiate()) {
        createPublisherOffer();
    }
}

bool TransportManagerImpl::setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
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

bool TransportManagerImpl::setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
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

bool TransportManagerImpl::setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config)
{
    return _subscriber.setConfiguration(config) && _publisher.setConfiguration(config);
}

bool TransportManagerImpl::addTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    if (track) {
        webrtc::RtpTransceiverInit init;
        init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
        return _publisher.addTransceiver(std::move(track), init);
    }
    return false;
}

bool TransportManagerImpl::removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    return _publisher.removeTrack(track);
}

void TransportManagerImpl::addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate)
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

void TransportManagerImpl::queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (callback && !closed()) {
        _publisher.querySenderStats(callback);
        _subscriber.queryReceiverStats(callback);
    }
}

void TransportManagerImpl::queryReceiverStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                                              const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (receiver && callback && !closed()) {
        _subscriber.queryReceiverStats(callback, receiver);
    }
}

void TransportManagerImpl::querySenderStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                                            const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (sender && callback && !closed()) {
        _publisher.querySenderStats(callback, sender);
    }
}

void TransportManagerImpl::setAudioPlayout(bool playout)
{
    _publisher.setAudioPlayout(playout);
    _subscriber.setAudioPlayout(playout);
}

void TransportManagerImpl::setAudioRecording(bool recording)
{
    _publisher.setAudioRecording(recording);
    _subscriber.setAudioRecording(recording);
}

void TransportManagerImpl::close()
{
    _publisher.close();
    _subscriber.close();
    stopPing();
    updateState();
}

void TransportManagerImpl::setListener(TransportManagerListener* listener)
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

void TransportManagerImpl::createPublisherOffer()
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

bool TransportManagerImpl::canNegotiate() const noexcept
{
    return !_subscriberPrimary || _fastPublish;
}

Transport& TransportManagerImpl::primaryTransport() noexcept
{
    return _subscriberPrimary ? _subscriber : _publisher;
}

const Transport& TransportManagerImpl::primaryTransport() const noexcept
{
    return _subscriberPrimary ? _subscriber : _publisher;
}

bool TransportManagerImpl::isPrimary(SignalTarget target) const noexcept
{
    return target == primaryTransport().target();;
}

void TransportManagerImpl::updateState()
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

void TransportManagerImpl::onSdpCreated(SignalTarget target,
                                        std::unique_ptr<webrtc::SessionDescriptionInterface> desc)
{
    if (SignalTarget::Publisher == target && desc && (!_prefferedAudioEncoder.empty() || !_prefferedVideoEncoder.empty())) {
        SdpPatch patch(desc.get());
        patch.setCodec(_prefferedAudioEncoder, cricket::MEDIA_TYPE_AUDIO);
        patch.setCodec(_prefferedVideoEncoder, cricket::MEDIA_TYPE_VIDEO);
    }
    switch (target) {
        case SignalTarget::Publisher:
            _publisher.setLocalDescription(std::move(desc));
            break;
        case SignalTarget::Subscriber:
            _subscriber.setLocalDescription(std::move(desc));
            break;
    }
}

void TransportManagerImpl::onSdpCreationFailure(SignalTarget target, webrtc::SdpType type,
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

void TransportManagerImpl::onSdpSet(SignalTarget target, bool local,
                                    const webrtc::SessionDescriptionInterface* desc)
{
    if (local) {
        if (desc) {
            std::string sdp;
            if (desc->ToString(&sdp)) {
                // offer is always from publisher (see also [negotiate])
                // answer from subscriber
                switch (target) {
                    case SignalTarget::Publisher:
                        _listener.invoke(&TransportManagerListener::onPublisherOffer,
                                         desc->type(), std::move(sdp));
                        break;
                    case SignalTarget::Subscriber:
                        _listener.invoke(&TransportManagerListener::onSubscriberAnswer,
                                         desc->type(), std::move(sdp));
                        break;
                }
            }
            else {
                logError("Failed to serialize " + desc->type() + std::string(" SDP for ") + toString(target));
            }
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

void TransportManagerImpl::onSdpSetFailure(SignalTarget target, bool local, webrtc::RTCError error)
{
    if (canLogError()) {
        logError("Failed to set of " + std::string(local ? "local SDP for " : "remote SDP for ") +
                 toString(target) + ": " +
                 error.message());
    }
    _listener.invoke(&TransportManagerListener::onSdpOperationFailed, target, std::move(error));
}

void TransportManagerImpl::onTransceiverAdded(SignalTarget target,
                                              rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Publisher == target && transceiver) {
        _listener.invoke(&TransportManagerListener::onLocalTrackAdded, transceiver->sender());
    }
}

void TransportManagerImpl::onTransceiverAddFailure(SignalTarget target,
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

void TransportManagerImpl::onLocalTrackRemoved(SignalTarget target,
                                               const std::string& id,
                                               cricket::MediaType type,
                                               const std::vector<std::string>&)
{
    if (SignalTarget::Publisher == target) {
        _listener.invoke(&TransportManagerListener::onLocalTrackRemoved, id, type);
    }
}

void TransportManagerImpl::onLocalDataChannelCreated(SignalTarget target,
                                                     rtc::scoped_refptr<DataChannel> channel)
{
    if (SignalTarget::Publisher == target && channel) {
        const auto label = channel->label();
        if (DataChannel::lossyLabel() == label || DataChannel::reliableLabel() == label) {
            _embeddedDCCount.fetch_add(1U);
            if (localDataChannelsAreCreated() && _pendingNegotiation.exchange(false)) {
                logVerbose("all DCs have been create, we have a pending offer - let's try to create it");
                createPublisherOffer();
            }
        }
        _listener.invoke(&TransportManagerListener::onLocalDataChannelCreated, std::move(channel));
    }
}

void TransportManagerImpl::onConnectionChange(SignalTarget target,
                                              webrtc::PeerConnectionInterface::PeerConnectionState)
{
    if (isPrimary(target)) {
        updateState();
    }
}

void TransportManagerImpl::onIceConnectionChange(SignalTarget target,
                                                 webrtc::PeerConnectionInterface::IceConnectionState)
{
    if (isPrimary(target)) {
        updateState();
    }
}

void TransportManagerImpl::onSignalingChange(SignalTarget target,
                                             webrtc::PeerConnectionInterface::SignalingState)
{
    if (isPrimary(target)) {
        updateState();
    }
}

void TransportManagerImpl::onNegotiationNeededEvent(SignalTarget target, uint32_t /*eventId*/)
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

void TransportManagerImpl::onRemoteDataChannelOpened(SignalTarget target,
                                                     rtc::scoped_refptr<DataChannel> channel)
{
    if (SignalTarget::Subscriber == target) {
        // in subscriber primary mode, server side opens sub data channels
        _listener.invoke(&TransportManagerListener::onRemoteDataChannelOpened, std::move(channel));
    }
}

void TransportManagerImpl::onIceCandidateGathered(SignalTarget target,
                                                  const webrtc::IceCandidateInterface* candidate)
{
    if (candidate) {
        _listener.invoke(&TransportManagerListener::onIceCandidateGathered, target,
                         candidate->sdp_mid(), candidate->sdp_mline_index(),
                         candidate->candidate());
    }
}

void TransportManagerImpl::onRemoteTrackAdded(SignalTarget target,
                                              rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                                              const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{
    if (SignalTarget::Subscriber == target && receiver) {
        std::string participantSid, streamId, trackId = receiver->id();
        if (!streams.empty() && streams.front()) {
            auto parts = unpackStreamId(streams.front()->id());
            participantSid = std::move(parts.first);
            streamId = std::move(parts.second);
            // firefox will get streamId (pID|trackId) instead of (pID|streamId) as it doesn't support sync tracks by stream
            // and generates its own track id instead of infer from sdp track id.
            if (!streamId.empty() && startWith(streamId, "TR")) {
                trackId = std::move(streamId);
            }
        }
        _listener.invoke(&TransportManagerListener::onRemoteTrackAdded,
                         std::move(receiver), std::move(trackId), std::move(participantSid));
    }
}

void TransportManagerImpl::onRemotedTrackRemoved(SignalTarget target,
                                                 rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (SignalTarget::Subscriber == target) {
        _listener.invoke(&TransportManagerListener::onRemotedTrackRemoved, std::move(receiver));
    }
}

bool TransportManagerImpl::onPingRequested()
{
    return _listener.invokeR<bool>(&PingPongKitListener::onPingRequested);
}

void TransportManagerImpl::onPongTimeout()
{
    _listener.invoke(&PingPongKitListener::onPongTimeout);
}


} // namespace LiveKitCpp
