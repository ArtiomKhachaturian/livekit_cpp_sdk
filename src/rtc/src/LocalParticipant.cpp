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
#include "LocalParticipant.h"
#include "AdmProxyFacade.h"
#include "AudioDeviceImpl.h"
#include "Blob.h"
#include "LocalVideoDeviceImpl.h"
#include "DataChannel.h"
#include "RtcUtils.h"
#include "PeerConnectionFactory.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"

#include <iostream>

namespace LiveKitCpp
{

LocalParticipant::LocalParticipant(PeerConnectionFactory* pcf,
                                   const Participant* session,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Base(logger)
    , _pcf(pcf)
    , _session(session)
{
}

void LocalParticipant::reset()
{
    _session(nullptr);
    clear(_audioTracks);
    clear(_videoTracks);
}

std::optional<bool> LocalParticipant::stereoRecording() const
{
    if (_pcf) {
        if (const auto admProxy = _pcf->admProxy().lock()) {
            return admProxy->recordingState().stereo();
        }
    }
    return std::nullopt;
}

size_t LocalParticipant::audioTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    return _audioTracks->size();
}

size_t LocalParticipant::videoTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    return _videoTracks->size();
}

std::shared_ptr<LocalAudioTrackImpl> LocalParticipant::addAudioTrack(std::shared_ptr<AudioDevice> device,
                                                                     EncryptionType encryption,
                                                                     const std::weak_ptr<TrackManager>& trackManager)
{
    if (auto audio = std::dynamic_pointer_cast<AudioDeviceImpl>(device)) {
        auto track = std::make_shared<LocalAudioTrackImpl>(encryption, std::move(audio),  trackManager, true);
        addTrack(track, _audioTracks);
        return track;
    }
    return {};
}

std::shared_ptr<LocalVideoTrackImpl> LocalParticipant::addVideoTrack(std::shared_ptr<LocalVideoDevice> device,
                                                                     EncryptionType encryption,
                                                                     const std::weak_ptr<TrackManager>& trackManager)
{
    if (auto video = std::dynamic_pointer_cast<LocalVideoDeviceImpl>(device)) {
        auto track = std::make_shared<LocalVideoTrackImpl>(encryption, std::move(video),  trackManager);
        addTrack(track, _videoTracks);
        return track;
    }
    return {};
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalParticipant::
    removeAudioTrack(std::shared_ptr<LocalAudioTrack> track)
{
    if (auto local = std::dynamic_pointer_cast<LocalAudioTrackImpl>(track)) {
        LOCK_WRITE_SAFE_OBJ(_audioTracks);
        for (auto it = _audioTracks->begin(); it != _audioTracks->end(); ++it) {
            if (*it == local) {
                _audioTracks->erase(it);
                return local->media();
            }
        }
    }
    return {};
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalParticipant::
    removeVideoTrack(std::shared_ptr<LocalVideoTrack> track)
{
    if (const auto local = std::dynamic_pointer_cast<LocalVideoTrackImpl>(track)) {
        LOCK_WRITE_SAFE_OBJ(_videoTracks);
        for (auto it = _videoTracks->begin(); it != _videoTracks->end(); ++it) {
            if (*it == track) {
                _videoTracks->erase(it);
                return local->media();
            }
        }
    }
    return {};
}

std::shared_ptr<LocalAudioTrack> LocalParticipant::audioTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    if (index < _audioTracks->size()) {
        return _audioTracks->at(index);
    }
    return {};
}

std::shared_ptr<LocalVideoTrack> LocalParticipant::videoTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    if (index < _videoTracks->size()) {
        return _videoTracks->at(index);
    }
    return {};
}

std::vector<std::shared_ptr<LocalTrackAccessor>> LocalParticipant::tracks() const
{
    std::vector<std::shared_ptr<LocalTrackAccessor>> tracks;
    LOCK_READ_SAFE_OBJ(_audioTracks);
    LOCK_READ_SAFE_OBJ(_videoTracks);
    tracks.reserve(_audioTracks->size() + _videoTracks->size());
    for (const auto& track : _audioTracks.constRef()) {
        if (auto localTrack = std::dynamic_pointer_cast<LocalTrackAccessor>(track)) {
            tracks.push_back(std::move(localTrack));
        }
    }
    for (const auto& track : _videoTracks.constRef()) {
        if (auto localTrack = std::dynamic_pointer_cast<LocalTrackAccessor>(track)) {
            tracks.push_back(std::move(localTrack));
        }
    }
    return tracks;
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> LocalParticipant::media() const
{
    const auto tracks = this->tracks();
    if (const auto size = tracks.size()) {
        std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> medias;
        for (const auto& track : tracks) {
            medias.push_back(track->media());
        }
        return medias;
    }
    return {};
}

std::shared_ptr<LocalTrackAccessor> LocalParticipant::track(const std::string& id, bool cid,
                                                            const std::optional<cricket::MediaType>& hint) const
{
    std::shared_ptr<LocalTrackAccessor> result;
    if (!id.empty()) {
        if (hint.has_value()) {
            switch (hint.value()) {
                case cricket::MEDIA_TYPE_AUDIO:
                    result = lookup(id, cid, _audioTracks);
                    break;
                case cricket::MEDIA_TYPE_VIDEO:
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

std::shared_ptr<LocalTrackAccessor> LocalParticipant::
    track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (sender) {
        return track(sender->id(), true, sender->media_type());
    }
    return {};
}

void LocalParticipant::setInfo(const ParticipantInfo& info)
{
    // update
    for (const auto& track : info._tracks) {
        if (TrackType::Audio == track._type) {
            if (const auto t = lookup(track._sid, false, _audioTracks)) {
                t->setSid(track._sid);
                t->setRemoteSideMute(track._muted);
            }
        }
        else if (TrackType::Video == track._type) {
            if (const auto t = lookup(track._sid, false, _videoTracks)) {
                t->setSid(track._sid);
                t->setRemoteSideMute(track._muted);
            }
        }
    }
    if (exchangeVal(info._sid, _sid)) {
        notify(&ParticipantListener::onSidChanged);
    }
    if (exchangeVal(info._identity, _identity)) {
        notify(&ParticipantListener::onIdentityChanged);
    }
    if (exchangeVal(info._name, _name)) {
        notify(&ParticipantListener::onNameChanged);
    }
    if (exchangeVal(info._metadata, _metadata)) {
        notify(&ParticipantListener::onMetadataChanged);
    }
    if (exchangeVal(info._kind, _kind)) {
        notify(&ParticipantListener::onKindChanged);
    }
}

bool LocalParticipant::setRemoteSideTrackMute(const std::string& trackSid, bool mute)
{
    if (!trackSid.empty()) {
        if (const auto track = lookup(trackSid, false, _audioTracks)) {
            track->setRemoteSideMute(mute);
            return true;
        }
        if (const auto track = lookup(trackSid, false, _videoTracks)) {
            track->setRemoteSideMute(mute);
            return true;
        }
    }
    return false;
}

void LocalParticipant::setSpeakerChanges(float level, bool active) const
{
    notify(&ParticipantListener::onSpeakerInfoChanged, level, active);
}

void LocalParticipant::setConnectionQuality(ConnectionQuality quality,
                                                           float score)
{
    notify(&ParticipantListener::onConnectionQualityChanged, quality, score);
}

template <class TTracks>
std::shared_ptr<LocalTrackAccessor> LocalParticipant::lookup(const std::string& id,
                                                             bool cid,
                                                             const TTracks& tracks)
{
    if (!id.empty()) {
        const std::lock_guard guard(tracks.mutex());
        for (const auto& track : tracks.constRef()) {
            const auto local = std::dynamic_pointer_cast<LocalTrackAccessor>(track);
            if (local && id == (cid ? local->cid() : track->sid())) {
                return local;
            }
        }
    }
    return {};
}

template <class TTrack, class TTracks>
void LocalParticipant::addTrack(const std::shared_ptr<TTrack>& track, TTracks& tracks)
{
    if (track) {
        const std::lock_guard guard(tracks.mutex());
        tracks->push_back(track);
    }
}

template <class TTracks>
void LocalParticipant::clear(TTracks& tracks)
{
    const std::lock_guard guard(tracks.mutex());
    for (const auto& track : tracks.constRef()) {
        if (const auto local = std::dynamic_pointer_cast<LocalTrackAccessor>(track)) {
            local->close();
        }
    }
    tracks->clear();
}

template <class Method, typename... Args>
void LocalParticipant::notify(const Method& method, Args&&... args) const
{
    LOCK_READ_SAFE_OBJ(_session);
    if (const auto session = _session.constRef()) {
        _listener.invoke(method, session, std::forward<Args>(args)...);
    }
}

void LocalParticipant::onEncryptionStateChanged(cricket::MediaType mediaType,
                                                const std::string&,
                                                const std::string& trackId,
                                                AesCgmCryptorState state)
{
    if (const auto err = toCryptoError(state)) {
        notify(&ParticipantListener::onTrackCryptoError,
               mediaTypeToTrackType(mediaType),
               EncryptionType::Gcm, trackId, err.value());
    }
}

} // namespace LiveKitCpp
