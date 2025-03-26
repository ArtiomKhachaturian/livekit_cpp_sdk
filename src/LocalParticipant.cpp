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
#include "LocalParticipant.h"
#include "Blob.h"
#include "CameraVideoTrack.h"
#include "CameraVideoSource.h"
#include "CameraManager.h"
#include "DataChannel.h"
#include "Utils.h"
#include "PeerConnectionFactory.h"
#include "rtc/ParticipantInfo.h"

namespace {

using namespace LiveKitCpp;

inline webrtc::scoped_refptr<webrtc::AudioTrackInterface>
    createMic(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf) {
    if (pcf) {
        cricket::AudioOptions options; // TODO: should be a part of room config
        if (const auto source = pcf->CreateAudioSource(options)) {
            return pcf->CreateAudioTrack(makeUuid(), source.get());
        }
    }
    return {};
}

inline webrtc::scoped_refptr<CameraVideoTrack>
    createCamera(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                 const std::shared_ptr<Bricks::Logger>& logger) {
    if (pcf && CameraManager::available()) {
        auto source = webrtc::make_ref_counted<CameraVideoSource>(pcf->signalingThread(),
                                                                  logger);
        return webrtc::make_ref_counted<CameraVideoTrack>(makeUuid(),
                                                          std::move(source),
                                                          logger);
    }
    return {};
}

}

namespace LiveKitCpp
{

LocalParticipant::LocalParticipant(TrackManager* manager,
                                   const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<Participant>(logger)
    , _manager(manager)
    , _pcf(pcf)
{
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

std::shared_ptr<LocalAudioTrackImpl> LocalParticipant::addMicrophoneTrack()
{
    if (auto mic = createMic(_pcf)) {
        const auto track = std::make_shared<LocalAudioTrackImpl>(std::move(mic),
                                                                 _manager, true,
                                                                 logger());
        addTrack(track);
        return track;
    }
    return {};
}

std::shared_ptr<CameraTrackImpl> LocalParticipant::addCameraTrack()
{
    if (auto camera = createCamera(_pcf, logger())) {
        const auto track = std::make_shared<CameraTrackImpl>(std::move(camera),
                                                             _manager, logger());
        addTrack(track);
        return track;
    }
    return {};
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalParticipant::
    removeAudioTrack(const std::shared_ptr<AudioTrack>& track)
{
    if (const auto local = std::dynamic_pointer_cast<LocalAudioTrackImpl>(track)) {
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
    removeVideoTrack(const std::shared_ptr<VideoTrack>& track)
{
    if (const auto local = std::dynamic_pointer_cast<LocalTrack>(track)) {
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

std::shared_ptr<AudioTrack> LocalParticipant::audioTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    if (index < _audioTracks->size()) {
        return _audioTracks->at(index);
    }
    return {};
}

std::shared_ptr<VideoTrack> LocalParticipant::videoTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    if (index < _videoTracks->size()) {
        return _videoTracks->at(index);
    }
    return {};
}

std::vector<std::shared_ptr<LocalTrack>> LocalParticipant::tracks() const
{
    std::vector<std::shared_ptr<LocalTrack>> tracks;
    LOCK_READ_SAFE_OBJ(_audioTracks);
    LOCK_READ_SAFE_OBJ(_videoTracks);
    tracks.reserve(_audioTracks->size() + _videoTracks->size());
    tracks.assign(_audioTracks->begin(), _audioTracks->end());
    for (const auto& track : _videoTracks.constRef()) {
        if (auto videoTrack = std::dynamic_pointer_cast<LocalTrack>(track)) {
            tracks.push_back(std::move(videoTrack));
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

std::shared_ptr<LocalTrack> LocalParticipant::track(const std::string& id, bool cid,
                                                    const std::optional<cricket::MediaType>& hint) const
{
    std::shared_ptr<LocalTrack> result;
    if (!id.empty()) {
        if (hint.has_value()) {
            switch (hint.value()) {
                case cricket::MEDIA_TYPE_AUDIO:
                    result = lookupAudio(id, cid);
                    break;
                case cricket::MEDIA_TYPE_VIDEO:
                    result = lookupVideo(id, cid);
                    break;
                default:
                    break;
            }
        }
        else {
            result = lookupAudio(id, cid);
            if (!result) {
                result = lookupVideo(id, cid);
            }
        }
    }
    return result;
}

std::shared_ptr<LocalTrack> LocalParticipant::
    track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (sender) {
        return track(sender->id(), true, sender->media_type());
    }
    return {};
}

bool LocalParticipant::setInfo(const ParticipantInfo& info)
{
    bool changed = false;
    if (exchangeVal(info._sid, _sid)) {
        changed = true;
    }
    if (exchangeVal(info._identity, _identity)) {
        changed = true;
    }
    if (exchangeVal(info._name, _name)) {
        changed = true;
    }
    if (exchangeVal(info._metadata, _metadata)) {
        changed = true;
    }
    if (exchangeVal(info._kind, _kind)) {
        changed = true;
    }
    return changed;
}

void LocalParticipant::addTrack(const std::shared_ptr<LocalAudioTrackImpl>& track)
{
    if (track) {
        LOCK_WRITE_SAFE_OBJ(_audioTracks);
        _audioTracks->push_back(track);
    }
}

void LocalParticipant::addTrack(const std::shared_ptr<VideoTrack>& track)
{
    if (track) {
        LOCK_WRITE_SAFE_OBJ(_videoTracks);
        _videoTracks->push_back(track);
    }
}

std::shared_ptr<LocalTrack> LocalParticipant::lookupAudio(const std::string& id,
                                                          bool cid) const
{
    if (!id.empty()) {
        LOCK_READ_SAFE_OBJ(_audioTracks);
        for (const auto& track : _audioTracks.constRef()) {
            if (id == (cid ? track->cid() : track->sid())) {
                return track;
            }
        }
    }
    return {};
}

std::shared_ptr<LocalTrack> LocalParticipant::lookupVideo(const std::string& id,
                                                          bool cid) const
{
    if (!id.empty()) {
        LOCK_READ_SAFE_OBJ(_videoTracks);
        for (const auto& track : _videoTracks.constRef()) {
            const auto videoTrack = std::dynamic_pointer_cast<LocalTrack>(track);
            if (videoTrack && id == (cid ? videoTrack->cid() : track->sid())) {
                return videoTrack;
            }
        }
    }
    return {};
}

} // namespace LiveKitCpp
#endif
