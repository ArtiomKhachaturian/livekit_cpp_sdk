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
#include "Loggable.h"
#include "SafeScopedRefPtr.h"
#include "rtc/AddTrackRequest.h"
#include "Track.h"
#include "LocalTrack.h"
#include "LocalTrackManager.h"
#include "Utils.h"
#include <api/media_stream_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

template<class TMediaTrack, class TBaseInterface>
class LocalTrackImpl : public Bricks::LoggableS<TBaseInterface>,
                       public LocalTrack,
                       private webrtc::ObserverInterface
{
    static_assert(std::is_base_of_v<Track, TBaseInterface>);
public:
    ~LocalTrackImpl() override { resetMedia(); }
    // client track ID, equal to WebRTC track ID
    const std::string& cid() const noexcept { return _cid; }
    // track name
    const std::string& name() const noexcept { return _name; }
    // impl. of LocalTrack
    void setSid(const std::string& sid) final { _sid(sid); }
    void resetMedia(bool remove = true) final;
    void addToTransport() final;
    void notifyThatMediaAddedToTransport() final;
    bool canPublish() const noexcept final;
    void fillRequest(AddTrackRequest* request) const override;
    // impl. of Track
    bool live() const final;
    void mute(bool mute) final; // request media track creation if needed
    bool muted() const final { return _muted; }
    std::string sid() const final { return _sid(); }
    bool remote() const noexcept final { return false; }
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
    const std::string _cid;
    const std::string _name;
    LocalTrackManager* const _manager;
    std::atomic_bool _muted = true;
    Bricks::SafeObj<std::string> _sid;
    SafeScopedRefPtr<TMediaTrack> _track;
    // under lock together with [_track]
    bool _pendingPublish = false;
};

template<class TMediaTrack, class TBaseInterface>
inline LocalTrackImpl<TMediaTrack, TBaseInterface>::
    LocalTrackImpl(std::string name, LocalTrackManager* manager,
                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TBaseInterface>(logger)
    , _cid(makeUuid())
    , _name(std::move(name))
    , _manager(manager)
{
}

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::resetMedia(bool remove)
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

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::addToTransport()
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

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::notifyThatMediaAddedToTransport()
{
    LOCK_WRITE_SAFE_OBJ(_track);
    if (_pendingPublish) {
        _pendingPublish = false;
        _track.constRef()->RegisterObserver(this);
        checkAuthorization();
        notifyAboutMuted(muted());
    }
}

template<class TMediaTrack, class TBaseInterface>
inline bool LocalTrackImpl<TMediaTrack, TBaseInterface>::canPublish() const noexcept
{
    LOCK_READ_SAFE_OBJ(_track);
    return !_pendingPublish && nullptr != _track.constRef();
}

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::
    fillRequest(AddTrackRequest* request) const
{
    if (request) {
        request->_cid = cid();
        request->_name = name();
        request->_muted = muted();
        request->_sid = sid();
    }
}

template<class TMediaTrack, class TBaseInterface>
inline bool LocalTrackImpl<TMediaTrack, TBaseInterface>::live() const
{
    if (const auto track = mediaTrack()) {
        return webrtc::MediaStreamTrackInterface::kLive == track->state();
    }
    return false;
}

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::mute(bool mute)
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

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::
    notifyAboutMuted(bool mute) const
{
    if (_manager && !_pendingPublish) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            _manager->notifyAboutMuteChanges(sid, mute);
        }
    }
}

template<class TMediaTrack, class TBaseInterface>
inline void LocalTrackImpl<TMediaTrack, TBaseInterface>::checkAuthorization()
{
    if (canPublish()) {
        requestAuthorization();
    }
}

} // namespace LiveKitCpp
