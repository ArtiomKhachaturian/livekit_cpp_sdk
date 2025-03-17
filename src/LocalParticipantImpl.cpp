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
#include "LocalParticipantImpl.h"
#include "rtc/TrackPublishedResponse.h"
#include "rtc/TrackUnpublishedResponse.h"

namespace LiveKitCpp
{

LocalParticipantImpl::LocalParticipantImpl(LocalTrackManager* manager,
                                           const std::shared_ptr<Bricks::Logger>& logger)
    : DataChannelsStorage<LocalTrackManager>(logger)
    , _manager(manager)
    , _microphone(this, true, logger)
    , _camera(this, logger)
{
}

LocalParticipantImpl::~LocalParticipantImpl()
{
    reset();
    clear();
}

bool LocalParticipantImpl::addDataChannel(rtc::scoped_refptr<DataChannel> channel)
{
    return channel && channel->local() && add(std::move(channel));
}

void LocalParticipantImpl::addTracksToTransport()
{
    _microphone.addToTransport();
    _camera.addToTransport();
}

void LocalParticipantImpl::reset()
{
    _microphone.resetMedia();
    _camera.resetMedia();
    _pendingLocalMedias({});
    setInfo({});
}

void LocalParticipantImpl::notifyThatTrackPublished(const TrackPublishedResponse& response)
{
    if (const auto t = track(response._cid, true)) {
        const auto& sid = response._track._sid;
        t->setSid(sid);
        // reconcile track mute status.
        // if server's track mute status doesn't match actual, we'll have to update
        // the server's copy
        const auto muted = t->muted();
        if (muted != response._track._muted) {
            notifyAboutMuteChanges(sid, muted);
        }
    }
}

void LocalParticipantImpl::notifyThatTrackUnpublished(const TrackUnpublishedResponse& response)
{
    if (const auto t = track(response._trackSid, false)) {
        t->resetMedia();
    }
}

LocalTrack* LocalParticipantImpl::track(const std::string& id, bool cid)
{
    if (!id.empty()) {
        if (id == (cid ? _microphone.cid() : _microphone.sid())) {
            return &_microphone;
        }
        if (id == (cid ? _camera.cid() : _camera.sid())) {
            return &_camera;
        }
    }
    return nullptr;
}

LocalTrack* LocalParticipantImpl::track(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    if (sender) {
        return track(sender->id(), true);
    }
    return nullptr;
}

std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>
    LocalParticipantImpl::pendingLocalMedia()
{
    LOCK_WRITE_SAFE_OBJ(_pendingLocalMedias);
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> medias;
    medias.reserve(_pendingLocalMedias->size());
    for (auto it = _pendingLocalMedias->begin(); it != _pendingLocalMedias->end(); ++it) {
        medias.push_back(std::move(it->second));
    }
    _pendingLocalMedias->clear();
    return medias;
}

std::string_view LocalParticipantImpl::logCategory() const
{
    static const std::string_view category("local_participant");
    return category;
}

bool LocalParticipantImpl::addLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    bool ok = false;
    if (_manager && track) {
        ok = _manager->addLocalMedia(track);
        if (!ok) {
            auto id = track->id();
            if (!id.empty()) {
                LOCK_WRITE_SAFE_OBJ(_pendingLocalMedias);
                _pendingLocalMedias->insert(std::make_pair(std::move(id), track));
                ok = true;
            }
        }
    }
    return ok;
}

bool LocalParticipantImpl::removeLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track)
{
    bool ok = false;
    if (_manager && track) {
        ok = _manager->removeLocalMedia(track);
        if (!ok) {
            const auto id = track->id();
            if (!id.empty()) {
                LOCK_WRITE_SAFE_OBJ(_pendingLocalMedias);
                ok = _pendingLocalMedias->erase(id) > 0U;
            }
        }
    }
    return ok;
}

webrtc::scoped_refptr<webrtc::AudioTrackInterface> LocalParticipantImpl::createMic(const std::string& label)
{
    if (_manager) {
        return _manager->createMic(label);
    }
    return {};
}

webrtc::scoped_refptr<CameraVideoTrack> LocalParticipantImpl::createCamera(const std::string& label)
{
    if (_manager) {
        return _manager->createCamera(label);
    }
    return {};
}

void LocalParticipantImpl::notifyAboutMuteChanges(const std::string& trackSid, bool muted)
{
    if (_manager) {
        _manager->notifyAboutMuteChanges(trackSid, muted);
    }
}

} // namespace LiveKitCpp
#endif
