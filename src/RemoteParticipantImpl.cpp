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
#include "RemoteParticipantImpl.h"
#include "RemoteAudioTrackImpl.h"
#include "RemoteVideoTrackImpl.h"
#include <api/media_stream_interface.h>
#include <optional>

namespace LiveKitCpp
{

RemoteParticipantImpl::RemoteParticipantImpl(const ParticipantInfo& info)
{
    setInfo(info);
}

bool RemoteParticipantImpl::addAudio(const std::string& sid, TrackManager* manager,
                                     const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (const auto audioTrack = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
        LOCK_READ_SAFE_OBJ(_info);
        if (const auto trackInfo = findBySid(sid)) {
            auto trackImpl = std::make_shared<RemoteAudioTrackImpl>(manager, *trackInfo, audioTrack);
            LOCK_WRITE_SAFE_OBJ(_audioTracks);
            _audioTracks->push_back(std::move(trackImpl));
            return true;
        }
    }
    return false;
}

bool RemoteParticipantImpl::addVideo(const std::string& sid, TrackManager* manager,
                                     const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (const auto videoTrack = dynamic_cast<webrtc::VideoTrackInterface*>(track.get())) {
        LOCK_READ_SAFE_OBJ(_info);
        if (const auto trackInfo = findBySid(sid)) {
            auto trackImpl = std::make_shared<RemoteVideoTrackImpl>(manager, *trackInfo, videoTrack);
            LOCK_WRITE_SAFE_OBJ(_videoTracks);
            _videoTracks->push_back(std::move(trackImpl));
            return true;
        }
    }
    return false;
}

bool RemoteParticipantImpl::removeMedia(const std::string& sid)
{
    bool ok = false;
    if (!sid.empty()) {
        {
            LOCK_WRITE_SAFE_OBJ(_audioTracks);
            ok = removeTrack(sid, _audioTracks.ref());
        }
        if (!ok) {
            LOCK_WRITE_SAFE_OBJ(_videoTracks);
            ok = removeTrack(sid, _videoTracks.ref());
        }
    }
    return ok;
}

size_t RemoteParticipantImpl::audioTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    return _audioTracks->size();
}

size_t RemoteParticipantImpl::videoTracksCount() const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    return _videoTracks->size();
}

std::shared_ptr<RemoteAudioTrack> RemoteParticipantImpl::audioTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_audioTracks);
    if (index < _audioTracks->size()) {
        return _audioTracks->at(index);
    }
    return {};
}

std::shared_ptr<RemoteVideoTrack> RemoteParticipantImpl::videoTrack(size_t index) const
{
    LOCK_READ_SAFE_OBJ(_videoTracks);
    if (index < _videoTracks->size()) {
        return _videoTracks->at(index);
    }
    return {};
}

const TrackInfo* RemoteParticipantImpl::findBySid(const std::string& sid) const
{
    if (!sid.empty()) {
        for (const auto& trackInfo : _info->_tracks) {
            if (trackInfo._sid == sid) {
                return &trackInfo;
            }
        }
    }
    return nullptr;
}

template<class TCollection>
bool RemoteParticipantImpl::removeTrack(const std::string& sid, TCollection& collection)
{
    if (!sid.empty()) {
        for (auto it = collection.begin(); it != collection.end(); ++it) {
            if (sid == (*it)->sid()) {
                collection.erase(it);
                return true;
            }
        }
    }
    return false;
}

} // namespace LiveKitCpp
