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
#include "LocalTrack.h"
#include "LocalTrackManager.h"
#include "Utils.h"
#include <api/media_stream_interface.h>

namespace LiveKitCpp
{

LocalTrack::LocalTrack(std::string name, LocalTrackManager* manager)
    : _cid(makeUuid())
    , _name(std::move(name))
    , _manager(manager)
{
}

LocalTrack::~LocalTrack()
{
    reset();
}

void LocalTrack::reset()
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (const auto track = _track.take()) {
        if (_manager) {
            _manager->removeLocalMedia(track);
        }
    }
}

void LocalTrack::setPublished(bool error)
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (error) {
        _track = {};
        _pendingPublish = false;
    }
    else if (_pendingPublish) {
        _pendingPublish = false;
        notifyAboutMuted(muted());
    }
}

bool LocalTrack::live() const noexcept
{
    LOCK_READ_SAFE_OBJ(_track);
    if (const auto& track = _track.constRef()) {
        return webrtc::MediaStreamTrackInterface::kLive == track->state();
    }
    return false;
}

void LocalTrack::fillRequest(AddTrackRequest& request) const
{
    switch (mediaType()) {
        case cricket::MEDIA_TYPE_AUDIO:
            request._type = TrackType::Audio;
            break;
        case cricket::MEDIA_TYPE_VIDEO:
            request._type = TrackType::Video;
            break;
        case cricket::MEDIA_TYPE_DATA:
            request._type = TrackType::Data;
            break;
        default:
            break;
    }
    request._cid = _cid;
    request._name = _name;
    request._muted = _muted;
    request._sid = _sid();
}

webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> LocalTrack::raw() const
{
    LOCK_READ_SAFE_OBJ(_track);
    return _track.constRef();
}

void LocalTrack::mute(bool mute)
{
    if (mute != _muted.exchange(mute)) {
        bool sendNotification = false;
        {
            LOCK_WRITE_SAFE_OBJ(_track);
            if (!mute && !_track.constRef() && _manager) {
                if (auto track = createMediaTrack(_cid)) {
                    // create track by demand
                    _pendingPublish = _manager->addLocalMedia(track);
                    _track = std::move(track);
                }
            }
            if (const auto& track = _track.constRef()) {
                const auto wasEnabled = track->enabled();
                if (wasEnabled == mute) {
                    track->set_enabled(!mute);
                    sendNotification = true;
                }
            }
        }
        if (sendNotification) {
            notifyAboutMuted(mute);
        }
    }
}

void LocalTrack::notifyAboutMuted(bool mute) const
{
    if (_manager && !_pendingPublish) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            _manager->notifyAboutMuteChanges(sid, mute);
        }
    }
}

} // namespace LiveKitCpp
