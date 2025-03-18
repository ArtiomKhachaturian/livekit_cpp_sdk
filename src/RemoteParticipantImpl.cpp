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
#include "RemoteParticipantImpl.h"
#include "RemoteAudioTrackImpl.h"
#include "RemoteVideoTrackImpl.h"
#include "Seq.h"
#include <api/media_stream_interface.h>
#include <optional>

namespace {

using namespace LiveKitCpp;

inline bool compareTrackInfo(const TrackInfo& l, const TrackInfo& r) {
    return l._sid == r._sid;
}

}

namespace LiveKitCpp
{

RemoteParticipantImpl::RemoteParticipantImpl(const ParticipantInfo& info)
{
    RemoteParticipantImpl::setInfo(info);
}

void RemoteParticipantImpl::reset()
{
    {
        LOCK_WRITE_SAFE_OBJ(_audioTracks);
        for (const auto& track : _audioTracks.take()) {
            _listeners.invoke(&RemoteParticipantListener::onAudioTrackRemoved,
                              this, track->sid());
        }
    }
    {
        LOCK_WRITE_SAFE_OBJ(_videoTracks);
        for (const auto& track : _videoTracks.take()) {
            _listeners.invoke(&RemoteParticipantListener::onVideoTrackRemoved,
                              this, track->sid());
        }
    }
}

std::optional<TrackType> RemoteParticipantImpl::trackType(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_info);
        if (const auto trackInfo = findBySid(sid)) {
            return trackInfo->_type;
        }
    }
    return std::nullopt;
}

bool RemoteParticipantImpl::addAudio(const std::string& sid, TrackManager* manager,
                                     const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    if (const auto audioTrack = dynamic_cast<webrtc::AudioTrackInterface*>(track.get())) {
        LOCK_READ_SAFE_OBJ(_info);
        if (const auto trackInfo = findBySid(sid)) {
            auto trackImpl = std::make_shared<RemoteAudioTrackImpl>(manager, *trackInfo, audioTrack);
            {
                LOCK_WRITE_SAFE_OBJ(_audioTracks);
                _audioTracks->push_back(std::move(trackImpl));
            }
            _listeners.invoke(&RemoteParticipantListener::onAudioTrackAdded, this, sid);
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
            {
                LOCK_WRITE_SAFE_OBJ(_videoTracks);
                _videoTracks->push_back(std::move(trackImpl));
            }
            _listeners.invoke(&RemoteParticipantListener::onVideoTrackAdded, this, sid);
            return true;
        }
    }
    return false;
}

bool RemoteParticipantImpl::removeAudio(const std::string& sid)
{
    bool ok = false;
    if (!sid.empty()) {
        LOCK_WRITE_SAFE_OBJ(_audioTracks);
        ok = removeTrack(sid, _audioTracks.ref());
    }
    if (ok) {
        _listeners.invoke(&RemoteParticipantListener::onAudioTrackRemoved, this, sid);
    }
    return ok;
}

bool RemoteParticipantImpl::removeVideo(const std::string& sid)
{
    bool ok = false;
    if (!sid.empty()) {
        LOCK_WRITE_SAFE_OBJ(_videoTracks);
        ok = removeTrack(sid, _videoTracks.ref());
    }
    if (ok) {
        _listeners.invoke(&RemoteParticipantListener::onVideoTrackRemoved, this, sid);
    }
    return ok;
}

void RemoteParticipantImpl::setInfo(const ParticipantInfo& info)
{
    const auto removed = Seq<TrackInfo>::difference(_info()._tracks,
                                                    info._tracks,
                                                    compareTrackInfo);
    _info(info);
    fireOnChanged();
    for (const auto& trackInfo : removed) {
        switch (trackInfo._type) {
            case TrackType::Audio:
                removeAudio(trackInfo._sid);
                break;
            case TrackType::Video:
                removeVideo(trackInfo._sid);
                break;
            default:
                break;
        }
    }
}

std::string RemoteParticipantImpl::sid() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_sid;
}

std::string RemoteParticipantImpl::identity() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_identity;
}

std::string RemoteParticipantImpl::name() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_name;
}

std::string RemoteParticipantImpl::metadata() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_metadata;
}

ParticipantKind RemoteParticipantImpl::kind() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_kind;
}

bool RemoteParticipantImpl::hasActivePublisher() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_isPublisher;
}

ParticipantState RemoteParticipantImpl::state() const
{
    LOCK_READ_SAFE_OBJ(_info);
    return _info->_state;
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

std::shared_ptr<RemoteAudioTrack> RemoteParticipantImpl::audioTrack(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_audioTracks);
        if (const auto ndx = findBySid(sid, _audioTracks.constRef())) {
            return _audioTracks->at(ndx.value());
        }
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

std::shared_ptr<RemoteVideoTrack> RemoteParticipantImpl::videoTrack(const std::string& sid) const
{
    if (!sid.empty()) {
        LOCK_READ_SAFE_OBJ(_videoTracks);
        if (const auto ndx = findBySid(sid, _videoTracks.constRef())) {
            return _videoTracks->at(ndx.value());
        }
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

template<class TTrack>
std::optional<size_t> RemoteParticipantImpl::findBySid(const std::string& sid,
                                                       const std::vector<TTrack>& collection)
{
    if (!sid.empty()) {
        if (const auto s = collection.size()) {
            for (size_t i = 0U; i < s; ++i) {
                if (sid == collection[i]->sid()) {
                    return i;
                }
            }
        }
    }
    return std::nullopt;
}

template<class TTrack>
bool RemoteParticipantImpl::removeTrack(const std::string& sid, std::vector<TTrack>& collection)
{
    if (const auto ndx = findBySid(sid, collection)) {
        collection.erase(collection.begin() + ndx.value());
        return true;
    }
    return false;
}

} // namespace LiveKitCpp
#endif
