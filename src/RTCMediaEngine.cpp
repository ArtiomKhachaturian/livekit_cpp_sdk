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
#include "LocalParticipantImpl.h"
#include "RemoteParticipants.h"
#include "PeerConnectionFactory.h"
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
                               std::shared_ptr<KeyProvider> e2eKeyProvider,
                               const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<SignalServerListener>(logger)
    , _signalingThread(pcf ? pcf->signalingThread() : std::weak_ptr<rtc::Thread>())
    , _e2eKeyProvider(std::move(e2eKeyProvider))
    , _localParticipant(new LocalParticipantImpl(this, pcf, logger))
    , _remoteParicipants(this, this, logger)
{
}

RTCMediaEngine::~RTCMediaEngine()
{
    RTCMediaEngine::cleanup();
}

std::shared_ptr<LocalParticipant> RTCMediaEngine::localParticipant() const
{
    return _localParticipant;
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>
    RTCMediaEngine::localTracks() const
{
    return _localParticipant->tracks();
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>
    RTCMediaEngine::localTrack(const std::string& id, bool cid) const
{
    if (const auto t = _localParticipant->track(id, cid)) {
        return t->rtcTrack();
    }
    return {};
}

void RTCMediaEngine::onJoin(const JoinResponse& response)
{
    const auto disconnectReason = response._participant._disconnectReason;
    if (DisconnectReason::UnknownReason == disconnectReason) {
        if (_e2eKeyProvider) {
            _e2eKeyProvider->setSifTrailer(response._sifTrailer);
        }
        _localParticipant->setInfo(response._participant);
        _remoteParicipants.setInfo(response._otherParticipants);
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
            if (info._sid == _localParticipant->sid()) {
                disconnectReason = info._disconnectReason;
                if (DisconnectReason::UnknownReason == disconnectReason) {
                    _localParticipant->setInfo(info);
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
    if (const auto t = _localParticipant->track(published._cid, true)) {
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

void RTCMediaEngine::cleanup(const std::optional<LiveKitError>& error, const std::string& what)
{
    _localParticipant->microphone().notifyThatMediaRemovedFromTransport();
    _localParticipant->camera().notifyThatMediaRemovedFromTransport();
    _remoteParicipants.reset();
    if (error && canLogError()) {
        logError(formatErrorMsg(error.value(), what));
    }
}

void RTCMediaEngine::onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (const auto track = _localParticipant->track(sender)) {
        bool encryption = false;
        if (_e2eKeyProvider) {
            auto codec = FrameCodec::create(sender->media_type(),
                                            _localParticipant->sid(),
                                            _signalingThread,
                                            _e2eKeyProvider,
                                            logger());
            if (codec) {
                sender->SetEncoderToPacketizerFrameTransformer(std::move(codec));
                encryption = true;
            }
        }
        track->notifyThatMediaAddedToTransport(encryption);
    }
}

void RTCMediaEngine::onLocalTrackAddFailure(const std::string& id,
                                            cricket::MediaType,
                                            const std::vector<std::string>&,
                                            webrtc::RTCError)
{
    if (const auto track = _localParticipant->track(id, true)) {
        track->notifyThatMediaRemovedFromTransport();
    }
}

void RTCMediaEngine::onLocalTrackRemoved(const std::string& id, cricket::MediaType)
{
    if (const auto track = _localParticipant->track(id, true)) {
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

void RTCMediaEngine::sendAddTrack(const LocalTrack* track)
{
    if (track && !closed()) {
        AddTrackRequest request;
        if (track->fillRequest(&request)) {
            if (SendResult::TransportError == sendAddTrack(request)) {
                // TODO: log error
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
            sendAddTrack(&_localParticipant->microphone());
            sendAddTrack(&_localParticipant->camera());
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

EncryptionType RTCMediaEngine::supportedEncryptionType() const
{
    return _e2eKeyProvider ? EncryptionType::Gcm : EncryptionType::None;
}

} // namespace LiveKitCpp
#endif
