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
#pragma once // LocalTrack.h
#include "SafeScopedRefPtr.h"
#include "rtc/AddTrackRequest.h"
#include "LocalTrack.h"
#include "LocalTrackManager.h"
#include <api/media_stream_interface.h>
#include <atomic>

namespace LiveKitCpp
{

template<class TMediaTrack>
class LocalTrackImpl : public LocalTrack
{
public:
    ~LocalTrackImpl() override { LocalTrackImpl<TMediaTrack>::resetMedia(); }
    // impl. of LocalTrack
    void resetMedia(bool remove = true) final;
    void notifyThaMediaAddedToTransport() final;
    bool live() const noexcept final;
    void fillRequest(AddTrackRequest* request) const override;
    // impl. of Track
    void mute(bool mute) final; // request media track creation if needed
    bool muted() const final { return _muted; }
protected:
    LocalTrackImpl(std::string name, LocalTrackManager* manager);
    LocalTrackManager* manager() const noexcept { return _manager; }
    virtual webrtc::scoped_refptr<TMediaTrack>
        createMediaTrack(const std::string& id) = 0;
private:
    void notifyAboutMuted(bool mute) const;
private:
    LocalTrackManager* const _manager;
    std::atomic_bool _muted = true;
    SafeScopedRefPtr<TMediaTrack> _track;
    // under lock together with [_track]
    bool _pendingPublish = false;
};

template<class TMediaTrack>
inline LocalTrackImpl<TMediaTrack>::LocalTrackImpl(std::string name,
                                                   LocalTrackManager* manager)
    : LocalTrack(std::move(name))
    , _manager(manager)
{
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::resetMedia(bool remove)
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (const auto track = _track.take()) {
        _pendingPublish = false;
        if (remove && _manager) {
            _manager->removeLocalMedia(track);
        }
    }
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::notifyThaMediaAddedToTransport()
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (_pendingPublish) {
        _pendingPublish = false;
        notifyAboutMuted(muted());
    }
}

template<class TMediaTrack>
inline bool LocalTrackImpl<TMediaTrack>::live() const noexcept
{
    LOCK_READ_SAFE_OBJ(_track);
    if (const auto& track = _track.constRef()) {
        return webrtc::MediaStreamTrackInterface::kLive == track->state();
    }
    return false;
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::fillRequest(AddTrackRequest* request) const
{
    if (request) {
        switch (mediaType()) {
            case cricket::MEDIA_TYPE_AUDIO:
                request->_type = TrackType::Audio;
                break;
            case cricket::MEDIA_TYPE_VIDEO:
                request->_type = TrackType::Video;
                break;
            case cricket::MEDIA_TYPE_DATA:
                request->_type = TrackType::Data;
                break;
            default:
                break;
        }
        request->_cid = cid();
        request->_name = name();
        request->_muted = muted();
        request->_sid = sid();
    }
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::mute(bool mute)
{
    if (mute != _muted.exchange(mute)) {
        bool sendNotification = false;
        {
            LOCK_WRITE_SAFE_OBJ(_track);
            if (!mute && !_track.constRef() && _manager) {
                if (auto track = createMediaTrack(cid())) {
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

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::notifyAboutMuted(bool mute) const
{
    if (_manager && !_pendingPublish) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            _manager->notifyAboutMuteChanges(sid, mute);
        }
    }
}

} // namespace LiveKitCpp
