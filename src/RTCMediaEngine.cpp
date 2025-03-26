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
#ifdef WEBRTC_AVAILABLE
#include "RTCMediaEngine.h"
#include "RemoteParticipants.h"
#include "PeerConnectionFactory.h"
#include "SessionListener.h"
#include "LiveKitError.h"
#include "Utils.h"
#include "FrameCodec.h"
#include "rtc/AddTrackRequest.h"
#include "rtc/MuteTrackRequest.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/TrackUnpublishedResponse.h"
#include "rtc/JoinResponse.h"
#include "rtc/ParticipantUpdate.h"
#include "rtc/TrackPublishedResponse.h"
#include "e2e/KeyProvider.h"

namespace {

using namespace LiveKitCpp;

inline std::string formatErrorMsg(LiveKitError error, const std::string& what) {
    auto msg = toString(error);
    if (!msg.empty() && !what.empty()) {
        msg += ": " + what;
    }
    return msg;
}

}

namespace LiveKitCpp
{

RTCMediaEngine::RTCMediaEngine(PeerConnectionFactory* pcf,
                               const Participant* session,
                               const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<SignalServerListener>(logger)
    , _session(session)
    , _signalingThread(pcf ? pcf->signalingThread() : std::weak_ptr<rtc::Thread>())
    , _localParticipant(this, webrtc::scoped_refptr<PeerConnectionFactory>(pcf), logger)
    , _remoteParicipants(this, this, logger)
{
}

RTCMediaEngine::~RTCMediaEngine()
{
    RTCMediaEngine::cleanup();
}

size_t RTCMediaEngine::localAudioTracksCount() const
{
    return _localParticipant.audioTracksCount();
}

size_t RTCMediaEngine::localVideoTracksCount() const
{
    return _localParticipant.videoTracksCount();
}

std::shared_ptr<LocalAudioTrackImpl> RTCMediaEngine::addLocalMicrophoneTrack()
{
    return _localParticipant.addMicrophoneTrack();
}

std::shared_ptr<CameraTrackImpl> RTCMediaEngine::addLocalCameraTrack()
{
    return _localParticipant.addCameraTrack();
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCMediaEngine::
    removeLocalAudioTrack(const std::shared_ptr<AudioTrack>& track)
{
    return _localParticipant.removeAudioTrack(track);
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> RTCMediaEngine::
    removeLocalVideoTrack(const std::shared_ptr<VideoTrack>& track)
{
    return _localParticipant.removeVideoTrack(track);
}

std::shared_ptr<AudioTrack> RTCMediaEngine::localAudioTrack(size_t index) const
{
    return _localParticipant.audioTrack(index);
}

std::shared_ptr<VideoTrack> RTCMediaEngine::localVideoTrack(size_t index) const
{
    return _localParticipant.videoTrack(index);
}

void RTCMediaEngine::enableAesCgmForLocalMedia(bool enable)
{
    _localParticipant.enableAesCgmForLocalMedia(enable);
}

bool RTCMediaEngine::aesCgmEnabledForLocalMedia() const noexcept
{
    return _localParticipant.aesCgmEnabledForLocalMedia();
}

void RTCMediaEngine::setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider)
{
    std::shared_ptr<KeyProvider> aesCgmKeyProvider;
    if (provider) {
        aesCgmKeyProvider.reset(provider.release());
        aesCgmKeyProvider->setSifTrailer(_sifTrailer());
    }
    std::atomic_store(&_aesCgmKeyProvider, std::move(aesCgmKeyProvider));
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>
    RTCMediaEngine::localTracks() const
{
    return _localParticipant.media();
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
    RTCMediaEngine::localTrack(const std::string& id, bool cid) const
{
    if (const auto t = _localParticipant.track(id, cid)) {
        return t->media();
    }
    return {};
}

void RTCMediaEngine::onJoin(const JoinResponse& response)
{
    const auto disconnectReason = response._participant._disconnectReason;
    if (DisconnectReason::UnknownReason == disconnectReason) {
        _sifTrailer(response._sifTrailer);
        if (const auto provider = std::atomic_load(&_aesCgmKeyProvider)) {
            provider->setSifTrailer(response._sifTrailer);
        }
        if (_localParticipant.setInfo(response._participant)) {
            callback(&ParticipantListener::onChanged, _session);
        }
        _remoteParicipants.setInfo(response._otherParticipants);
        notifyAboutLocalParticipantJoinLeave(true);
    }
    else {
        handleLocalParticipantDisconnection(disconnectReason);
    }
}

void RTCMediaEngine::onUpdate(const ParticipantUpdate& update)
{
    std::vector<ParticipantInfo> infos = update._participants;
    if (const auto s = infos.size()) {
        DisconnectReason disconnectReason = DisconnectReason::UnknownReason;
        for (size_t i = 0U; i < s; ++i) {
            const auto& info = infos.at(i);
            if (info._sid == _localParticipant.sid()) {
                disconnectReason = info._disconnectReason;
                if (DisconnectReason::UnknownReason == disconnectReason) {
                    if (_localParticipant.setInfo(info)) {
                        callback(&ParticipantListener::onChanged, _session);
                    }
                    infos.erase(infos.begin() + i);
                }
                break;
            }
        }
        if (DisconnectReason::UnknownReason == disconnectReason) {
            _remoteParicipants.updateInfo(infos);
        }
        else {
            handleLocalParticipantDisconnection(disconnectReason);
        }
    }
}

void RTCMediaEngine::onTrackPublished(const TrackPublishedResponse& published)
{
    if (const auto t = _localParticipant.track(published._cid, true)) {
        const auto& sid = published._track._sid;
        t->setSid(sid);
        // reconcile track mute status.
        // if server's track mute status doesn't match actual, we'll have to update
        // the server's copy
        const auto muted = t->muted();
        if (muted != published._track._muted) {
            notifyAboutMuteChanges(sid, muted);
        }
    }
}

void RTCMediaEngine::onReconnect(const ReconnectResponse& response)
{
    notifyAboutLocalParticipantJoinLeave(true);
}

void RTCMediaEngine::cleanup(const std::optional<LiveKitError>& error, const std::string& what)
{
    for (const auto& track : _localParticipant.tracks()) {
        track->notifyThatMediaRemovedFromTransport();
    }
    _remoteParicipants.reset();
    if (error && canLogError()) {
        logError(formatErrorMsg(error.value(), what));
    }
    notifyAboutLocalParticipantJoinLeave(false);
}

void RTCMediaEngine::onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (const auto track = _localParticipant.track(sender)) {
        if (auto codec = createCodec(true, sender->media_type(), sender->id())) {
            sender->SetFrameTransformer(std::move(codec));
            track->notifyThatMediaAddedToTransport(true);
        }
        else {
            track->notifyThatMediaAddedToTransport(false);
        }
        sendAddTrack(track);
    }
}

void RTCMediaEngine::onLocalTrackRemoved(const std::string& id, cricket::MediaType)
{
    if (const auto track = _localParticipant.track(id, true)) {
        track->notifyThatMediaRemovedFromTransport();
    }
}

void RTCMediaEngine::onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    _remoteParicipants.addMedia(transceiver);
}

void RTCMediaEngine::onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    _remoteParicipants.removeMedia(receiver);
}

void RTCMediaEngine::handleLocalParticipantDisconnection(DisconnectReason reason)
{
    if (DisconnectReason::UnknownReason != reason) {
        cleanup(toLiveKitError(reason));
    }
}

void RTCMediaEngine::notifyAboutLocalParticipantJoinLeave(bool join) const
{
    if (!_localParticipant.sid().empty()) {
        if (join) {
            callback(&SessionListener::onLocalParticipantJoined);
        }
        else {
            callback(&SessionListener::onLocalParticipantLeaved);
        }
    }
}

void RTCMediaEngine::sendAddTrack(const std::shared_ptr<LocalTrack>& track)
{
    if (track && !closed()) {
        AddTrackRequest request;
        if (track->fillRequest(&request)) {
            switch (sendAddTrack(request)) {
                case SendResult::Ok:
                    logVerbose("add local track '" + track->cid() +
                               "' request has been sent to server");
                    break;
                case SendResult::TransportError:
                    logError("failed to send add local track '" +
                             track->cid() + "' request to server");
                    break;
                default:
                    break;
            }
        }
    }
}

void RTCMediaEngine::onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState,
                                   webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                                   webrtc::PeerConnectionInterface::PeerConnectionState subscriberState)
{
    switch (publisherState) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
            // publish local tracks
            for (const auto& track : _localParticipant.tracks()) {
                sendAddTrack(track);
            }
            break;
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            for (const auto& track : _localParticipant.tracks()) {
                track->notifyThatMediaRemovedFromTransport();
            }
            break;
        default:
            break;
    }
    switch (subscriberState) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            _remoteParicipants.reset();
            break;
        default:
            break;
    }
}

webrtc::scoped_refptr<FrameCodec> RTCMediaEngine::createCodec(bool local,
                                                              cricket::MediaType mediaType,
                                                              std::string id) const
{
    if (const auto provider = std::atomic_load(&_aesCgmKeyProvider)) {
        return FrameCodec::create(mediaType, std::move(id), _signalingThread,
                                  provider, logger());
    }
    return {};
}

void RTCMediaEngine::notifyAboutMuteChanges(const std::string& trackSid, bool muted)
{
    if (!trackSid.empty()) {
        MuteTrackRequest request;
        request._sid = trackSid;
        request._muted = muted;
        const auto result = sendMuteTrack(request);
        if (SendResult::TransportError == result) {
            // TODO: log error
        }
    }
}

EncryptionType RTCMediaEngine::localEncryptionType() const
{
    if (_localParticipant.aesCgmEnabledForLocalMedia() &&
        std::atomic_load(&_aesCgmKeyProvider)) {
        return EncryptionType::Gcm;
    }
    return EncryptionType::None;
}

void RTCMediaEngine::onParticipantAdded(const std::string& sid)
{
    callback(&SessionListener::onRemoteParticipantAdded, sid);
}

void RTCMediaEngine::onParticipantRemoved(const std::string& sid)
{
    callback(&SessionListener::onRemoteParticipantRemoved, sid);
}

} // namespace LiveKitCpp
#endif
