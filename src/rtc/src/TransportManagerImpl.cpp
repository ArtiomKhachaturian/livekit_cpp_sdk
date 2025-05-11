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
#include "LocalAudioTrackImpl.h"
#include "LocalVideoTrackImpl.h"
#include "TrackInfoSeq.h"
#include "DataChannel.h"
#include "RoomUtils.h"
#include "RtcUtils.h"
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
                                           std::vector<TrackInfo> tracksInfo,
                                           const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                           const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                                           const std::weak_ptr<TrackManager>& trackManager,
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
    , _logCategory("transport_manager_" + identity)
    , _negotiationTimer(_negotiationDelay ? new MediaTimer(pcf) : nullptr)
    , _trackManager(trackManager)
    , _publisher(SignalTarget::Publisher, this, pcf, conf, identity, prefferedAudioEncoder, prefferedVideoEncoder, logger)
    , _subscriber(SignalTarget::Subscriber, this, pcf, conf, identity, {}, {}, logger)
    , _pingPongKit(positiveOrZero(pingInterval), positiveOrZero(pingTimeout), pcf)
    , _state(webrtc::PeerConnectionInterface::PeerConnectionState::kNew)
    , _tracksInfo(std::move(tracksInfo))
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

void TransportManagerImpl::addTrack(std::shared_ptr<AudioDeviceImpl> device, EncryptionType encryption)
{
    if (device) {
        webrtc::RtpTransceiverInit init;
        init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
        _publisher.addTrack(std::move(device), encryption, init);
    }
}

void TransportManagerImpl::addTrack(std::shared_ptr<LocalVideoDeviceImpl> device, EncryptionType encryption)
{
    if (device) {
        webrtc::RtpTransceiverInit init;
        init.direction = webrtc::RtpTransceiverDirection::kSendOnly;
        _publisher.addTrack(std::move(device), encryption, init);
    }
}

bool TransportManagerImpl::removeTrack(const std::string& id, bool cid)
{
    if (!id.empty()) {
        if (cid) {
            return _publisher.removeTrack(id);
        }
        if (const auto t = track(id, false)) {
            return _publisher.removeTrack(t->cid());
        }
    }
    return false;
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
    remove<webrtc::MediaType::AUDIO>(_audioTracks);
    remove<webrtc::MediaType::VIDEO>(_videoTracks);
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

void TransportManagerImpl::updateTracksInfo(std::vector<TrackInfo> tracksInfo)
{
    std::vector<TrackInfo> removed;
    {
        LOCK_WRITE_SAFE_OBJ(_tracksInfo);
        findDifference(_tracksInfo.constRef(), tracksInfo, nullptr, &removed);
        _tracksInfo = std::move(tracksInfo);
    }
    for (const auto& info : removed) {
        removeTrack(info._sid, false);
    }
}

bool TransportManagerImpl::setRemoteSideTrackMute(const std::string& trackSid, bool mute)
{
    if (const auto t = track(trackSid, false)) {
        t->setRemoteSideMute(mute);
        return true;
    }
    return false;
}

std::shared_ptr<LocalTrackAccessor> TransportManagerImpl::
    track(const std::string& id, bool cid, const std::optional<webrtc::MediaType>& hint) const
{
    std::shared_ptr<LocalTrackAccessor> result;
    if (!id.empty()) {
        if (hint.has_value()) {
            switch (hint.value()) {
                case webrtc::MediaType::AUDIO:
                    result = lookup(id, cid, _audioTracks);
                    break;
                case webrtc::MediaType::VIDEO:
                    result = lookup(id, cid, _videoTracks);
                    break;
                default:
                    break;
            }
        }
        else {
            result = lookup(id, cid, _audioTracks);
            if (!result) {
                result = lookup(id, cid, _videoTracks);
            }
        }
    }
    return result;
}

template <class TTracks>
std::shared_ptr<LocalTrackAccessor> TransportManagerImpl::lookup(const std::string& id,
                                                                 bool cid,
                                                                 const TTracks& tracks)
{
    if (!id.empty()) {
        const std::lock_guard guard(tracks.mutex());
        if (cid) {
            const auto it = tracks->find(id);
            if (it != tracks->end()) {
                return it->second;
            }
        }
        else {
            for (auto it = tracks->begin(); it != tracks->end(); ++it) {
                if (it->second->sid() == id) {
                    return it->second;
                }
            }
        }
    }
    return {};
}

template <webrtc::MediaType type, class TTracks>
void TransportManagerImpl::remove(const std::string& id, TTracks& tracks) const
{
    bool removed = false;
    if (!id.empty()) {
        const std::lock_guard guard(tracks.mutex());
        removed = tracks->erase(id) > 0U;
    }
    if (removed) {
        _listener.invoke(&TransportManagerListener::onLocalTrackRemoved, id, type);
    }
}

template <webrtc::MediaType type, class TTracks>
void TransportManagerImpl::remove(TTracks& tracks) const
{
    std::list<std::string> ids;
    {
        const std::lock_guard guard(tracks.mutex());
        for (auto it = tracks->begin(); it != tracks->end(); ++it) {
            ids.push_back(std::move(it->first));
        }
    }
    for (auto& id : ids) {
        _listener.invoke(&TransportManagerListener::onLocalTrackRemoved, std::move(id), type);
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
        if (canLogInfo()) {
            logInfo(makeStateChangesString(oldState, newState));
        }
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

void TransportManagerImpl::onLocalAudioTrackAdded(SignalTarget target,
                                                  std::shared_ptr<AudioDeviceImpl> device,
                                                  EncryptionType encryption,
                                                  rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Publisher == target && device && transceiver) {
        auto track = std::make_shared<LocalAudioTrackImpl>(std::move(device), encryption,
                                                           std::move(transceiver),
                                                           _trackManager);
        {
            LOCK_WRITE_SAFE_OBJ(_audioTracks);
            _audioTracks->insert(std::make_pair(track->id(), track));
        }
        _listener.invoke(&TransportManagerListener::onLocalAudioTrackAdded, track);
    }
}

void TransportManagerImpl::onLocalVideoTrackAdded(SignalTarget target,
                                                  std::shared_ptr<LocalVideoDeviceImpl> device,
                                                  EncryptionType encryption,
                                                  rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (SignalTarget::Publisher == target && device && transceiver) {
        auto track = std::make_shared<LocalVideoTrackImpl>(std::move(device), encryption,
                                                           std::move(transceiver),
                                                           _trackManager);
        {
            LOCK_WRITE_SAFE_OBJ(_videoTracks);
            _videoTracks->insert(std::make_pair(track->id(), track));
        }
        _listener.invoke(&TransportManagerListener::onLocalVideoTrackAdded, track);
    }
}

void TransportManagerImpl::onLocalTrackAddFailure(SignalTarget target, const std::string& id,
                                                  webrtc::MediaType type,
                                                  const webrtc::RtpTransceiverInit&,
                                                  webrtc::RTCError error)
{
    if (SignalTarget::Publisher == target) {
        _listener.invoke(&TransportManagerListener::onLocalTrackAddFailure, id, type, std::move(error));
    }
}

void TransportManagerImpl::onLocalTrackRemoved(SignalTarget target,
                                               const std::string& id,
                                               webrtc::MediaType type,
                                               const std::vector<std::string>&)
{
    if (SignalTarget::Publisher == target) {
        switch (type) {
            case webrtc::MediaType::AUDIO:
                remove<webrtc::MediaType::AUDIO>(id, _audioTracks);
                break;
            case webrtc::MediaType::VIDEO:
                remove<webrtc::MediaType::VIDEO>(id, _videoTracks);
                break;
            default:
                break;
        }
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
