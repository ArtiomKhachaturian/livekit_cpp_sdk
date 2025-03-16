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
#include "RTCMediaEngine.h"
#include "RemoteTrack.h"
#include "LocalParticipantImpl.h"
#include "rtc/AddTrackRequest.h"
#include "rtc/MuteTrackRequest.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/TrackUnpublishedResponse.h"
#include "rtc/JoinResponse.h"

namespace LiveKitCpp
{

RTCMediaEngine::RTCMediaEngine(const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<SignalServerListener>(logger)
    , _localParticipant(new LocalParticipantImpl(this, logger))
{
}

RTCMediaEngine::~RTCMediaEngine()
{
    cleanupLocalResources();
    cleanupRemoteResources();
}

std::shared_ptr<LocalParticipant> RTCMediaEngine::localParticipant() const
{
    return _localParticipant;
}

void RTCMediaEngine::addLocalResourcesToTransport()
{
    _localParticipant->addTracksToTransport();
}

void RTCMediaEngine::cleanupLocalResources()
{
    _localParticipant->resetTracksMedia();
}

void RTCMediaEngine::cleanupRemoteResources()
{
    //_remoteDCs({});
    _remoteTracks({});
}

void RTCMediaEngine::onJoin(const JoinResponse& response)
{
    _localParticipant->setInfo(response._participant);
}

void RTCMediaEngine::onTrackPublished(const TrackPublishedResponse& published)
{
    if (const auto track = _localParticipant->track(published._cid, true)) {
        track->setSid(published._track._sid);
        // reconcile track mute status.
        // if server's track mute status doesn't match actual, we'll have to update
        // the server's copy
        const auto muted = track->muted();
        if (muted != published._track._muted) {
            notifyAboutMuteChanges(published._track._sid, muted);
        }
    }
}

void RTCMediaEngine::onTrackUnpublished(const TrackUnpublishedResponse& unpublished)
{
    if (const auto track = _localParticipant->track(unpublished._trackSid, false)) {
        track->resetMedia();
    }
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> RTCMediaEngine::pendingLocalMedia()
{
    return _localParticipant->pendingLocalMedia();
}

void RTCMediaEngine::onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    if (const auto track = _localParticipant->track(sender)) {
        track->notifyThatMediaAddedToTransport();
        sendAddTrack(track);
    }
}

void RTCMediaEngine::onLocalTrackAddFailure(const std::string& id,
                                            cricket::MediaType,
                                            const std::vector<std::string>&,
                                            webrtc::RTCError)
{
    if (const auto track = _localParticipant->track(id, true)) {
        track->resetMedia();
    }
}

void RTCMediaEngine::onLocalTrackRemoved(const std::string& id, cricket::MediaType)
{
    if (const auto track = _localParticipant->track(id, true)) {
        track->resetMedia(false);
    }
}

void RTCMediaEngine::onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{
    if (transceiver) {
        TrackManager* manager = this;
        auto sid = RemoteTrack::sid(transceiver->receiver());
        if (!sid.empty()) {
            auto track = std::make_unique<RemoteTrack>(manager, transceiver->receiver());
            LOCK_WRITE_SAFE_OBJ(_remoteTracks);
            _remoteTracks->insert(std::make_pair(std::move(sid), std::move(track)));
        }
        else {
            logWarning("empty ID of received transceiver");
        }
    }
}

void RTCMediaEngine::onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (receiver) {
        const auto sid = receiver->id();
        LOCK_WRITE_SAFE_OBJ(_remoteTracks);
        _remoteTracks->erase(sid);
    }
}

void RTCMediaEngine::onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel)
{
    _localParticipant->add(std::move(channel));
}

void RTCMediaEngine::onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel)
{
    if (channel) {
        //LOCK_WRITE_SAFE_OBJ(_remoteDCs);
        //addDataChannelToList(std::move(channel), _remoteDCs.ref());
    }
}

void RTCMediaEngine::sendAddTrack(const LocalTrack* track)
{
    if (track && track->canPublish() && !closed()) {
        AddTrackRequest request;
        track->fillRequest(&request);
        if (SendResult::TransportError == sendAddTrack(request)) {
            // TODO: log error
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
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            cleanupLocalResources();
            break;
        default:
            break;
    }
    switch (subscriberState) {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
            cleanupRemoteResources();
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

} // namespace LiveKitCpp
