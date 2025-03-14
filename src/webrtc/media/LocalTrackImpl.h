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
class LocalTrackImpl : public LocalTrack, private webrtc::ObserverInterface
{
public:
    ~LocalTrackImpl() override { LocalTrackImpl<TMediaTrack>::resetMedia(); }
    // impl. of LocalTrack
    void resetMedia(bool remove = true) final;
    void addToTransport() final;
    void notifyThatMediaAddedToTransport() final;
    bool canPublish() const noexcept final;
    void fillRequest(AddTrackRequest* request) const override;
    // impl. of Track
    bool live() const final;
    void mute(bool mute) final; // request media track creation if needed
    bool muted() const final { return _muted; }
protected:
    LocalTrackImpl(std::string name, LocalTrackManager* manager,
                   const std::shared_ptr<Bricks::Logger>& logger = {});
    LocalTrackManager* manager() const noexcept { return _manager; }
    virtual void requestAuthorization() {}
    virtual webrtc::scoped_refptr<TMediaTrack> createMediaTrack(const std::string& id) = 0;
    webrtc::scoped_refptr<TMediaTrack> mediaTrack() const noexcept { return _track(); }
private:
    void notifyAboutMuted(bool mute) const;
    void checkAuthorization();
    // impl. of webrtc::NotifierInterface
    void OnChanged() final { checkAuthorization(); }
private:
    LocalTrackManager* const _manager;
    std::atomic_bool _muted = true;
    SafeScopedRefPtr<TMediaTrack> _track;
    // under lock together with [_track]
    bool _pendingPublish = false;
};

template<class TMediaTrack>
inline LocalTrackImpl<TMediaTrack>::LocalTrackImpl(std::string name,
                                                   LocalTrackManager* manager,
                                                   const std::shared_ptr<Bricks::Logger>& logger)
    : LocalTrack(std::move(name), logger)
    , _manager(manager)
{
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::resetMedia(bool remove)
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (const auto track = _track.take()) {
        if (!_pendingPublish) {
            track->UnregisterObserver(this);
        }
        else {
            _pendingPublish = false;
        }
        if (remove && _manager) {
            _manager->removeLocalMedia(track);
        }
    }
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::addToTransport()
{
    if (!muted() && _manager) {
        LOCK_WRITE_SAFE_OBJ(_track);
        if (!_track.constRef()) {
            if (auto track = createMediaTrack(cid())) {
                // create track by demand
                track->set_enabled(true);
                _pendingPublish = _manager->addLocalMedia(track);
                if (_pendingPublish) {
                    _track = std::move(track);
                }
            }
        }
    }
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::notifyThatMediaAddedToTransport()
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (_pendingPublish) {
        _pendingPublish = false;
        _track.constRef()->RegisterObserver(this);
        checkAuthorization();
        notifyAboutMuted(muted());
    }
}

template<class TMediaTrack>
inline bool LocalTrackImpl<TMediaTrack>::canPublish() const noexcept
{
    LOCK_READ_SAFE_OBJ(_track);
    return !_pendingPublish && nullptr != _track.constRef();
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::fillRequest(AddTrackRequest* request) const
{
    if (request) {
        request->_type = type();
        request->_cid = cid();
        request->_name = name();
        request->_muted = muted();
        request->_sid = sid();
        request->_source = source();
    }
}

template<class TMediaTrack>
inline bool LocalTrackImpl<TMediaTrack>::live() const
{
    if (const auto track = mediaTrack()) {
        return webrtc::MediaStreamTrackInterface::kLive == track->state();
    }
    return false;
}

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::mute(bool mute)
{
    if (mute != _muted.exchange(mute)) {
        bool sendNotification = false;
        {
            LOCK_WRITE_SAFE_OBJ(_track);
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

template<class TMediaTrack>
inline void LocalTrackImpl<TMediaTrack>::checkAuthorization()
{
    if (canPublish()) {
        requestAuthorization();
    }
}

} // namespace LiveKitCpp
