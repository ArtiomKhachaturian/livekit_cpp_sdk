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
#include "TrackManager.h"
#include <api/media_stream_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

template<class TRtcTrack, class TTrackApi>
class LocalTrackImpl : public Bricks::LoggableS<TTrackApi>,
                       public LocalTrack
{
    static_assert(std::is_base_of_v<Track, TTrackApi>);
    static_assert(std::is_base_of_v<webrtc::MediaStreamTrackInterface, TRtcTrack>);
public:
    ~LocalTrackImpl() override = default;
    // impl. of LocalTrack
    std::string cid() const final;
    void setSid(const std::string& sid) final { _sid(sid); }
    // server track ID if any
    std::string sid() const { return _sid(); }
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> rtcTrack() const final {
        return _mediaTrack;
    }
    void notifyThatMediaAddedToTransport(bool encryption) final;
    void notifyThatMediaRemovedFromTransport() final;
    bool fillRequest(AddTrackRequest* request) const override;
    // impl. of Track
    std::string id() const final { return cid(); }
    std::string name() const final { return _name; }
    bool live() const final;
    void mute(bool mute) final; // request media track creation if needed
    bool muted() const final { return _muted; }
    bool remote() const noexcept final { return false; }
protected:
    LocalTrackImpl(std::string name,
                   webrtc::scoped_refptr<TRtcTrack> mediaTrack,
                   TrackManager* manager,
                   const std::shared_ptr<Bricks::Logger>& logger = {});
    const auto& mediaTrack() const noexcept { return _mediaTrack; }
private:
    void notifyAboutMuted(bool mute) const;
    EncryptionType encryption() const;
private:
    const std::string _name;
    const webrtc::scoped_refptr<TRtcTrack> _mediaTrack;
    TrackManager* const _manager;
    std::atomic_bool _muted = true;
    std::atomic_bool _added = false;
    std::atomic_bool _encryption = false;
    Bricks::SafeObj<std::string> _sid;
};

template<class TRtcTrack, class TTrackApi>
inline LocalTrackImpl<TRtcTrack, TTrackApi>::
    LocalTrackImpl(std::string name,
                   webrtc::scoped_refptr<TRtcTrack> mediaTrack,
                   TrackManager* manager,
                   const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<TTrackApi>(logger)
    , _name(std::move(name))
    , _mediaTrack(std::move(mediaTrack))
    , _manager(manager)
{
    if (_mediaTrack) {
        _mediaTrack->set_enabled(!_muted);
    }
}

template<class TRtcTrack, class TTrackApi>
inline std::string LocalTrackImpl<TRtcTrack, TTrackApi>::cid() const
{
    return _mediaTrack ? _mediaTrack->id() : std::string{};
}

template<class TRtcTrack, class TTrackApi>
inline void LocalTrackImpl<TRtcTrack, TTrackApi>::
    notifyThatMediaAddedToTransport(bool encryption)
{
    if (!_added.exchange(true)) {
        _encryption = encryption;
        notifyAboutMuted(muted());
    }
}

template<class TRtcTrack, class TTrackApi>
inline void LocalTrackImpl<TRtcTrack, TTrackApi>::notifyThatMediaRemovedFromTransport()
{
    if (_added.exchange(false)) {
        _encryption = false;
    }
}

template<class TRtcTrack, class TTrackApi>
inline bool LocalTrackImpl<TRtcTrack, TTrackApi>::
    fillRequest(AddTrackRequest* request) const
{
    if (request && _mediaTrack && _added) {
        request->_encryption = encryption();
        request->_cid = cid();
        request->_name = name();
        request->_muted = muted();
        request->_sid = sid();
        return true;
    }
    return false;
}

template<class TRtcTrack, class TTrackApi>
inline bool LocalTrackImpl<TRtcTrack, TTrackApi>::live() const
{
    return _mediaTrack && webrtc::MediaStreamTrackInterface::kLive == _mediaTrack->state();
}

template<class TRtcTrack, class TTrackApi>
inline void LocalTrackImpl<TRtcTrack, TTrackApi>::mute(bool mute)
{
    if (mute != _muted.exchange(mute)) {
        bool sendNotification = false;
        if (_mediaTrack) {
            const auto wasEnabled = _mediaTrack->enabled();
            if (wasEnabled == mute) {
                _mediaTrack->set_enabled(!mute);
                sendNotification = _added;
            }
        }
        if (sendNotification) {
            notifyAboutMuted(mute);
        }
    }
}

template<class TRtcTrack, class TTrackApi>
inline void LocalTrackImpl<TRtcTrack, TTrackApi>::
    notifyAboutMuted(bool mute) const
{
    if (_manager && _added) {
        const auto sid = this->sid();
        if (!sid.empty()) {
            _manager->notifyAboutMuteChanges(sid, mute);
        }
    }
}

template<class TRtcTrack, class TTrackApi>
inline EncryptionType LocalTrackImpl<TRtcTrack, TTrackApi>::encryption() const
{
    if (_manager && _encryption) {
        return _manager->supportedEncryptionType();
    }
    return EncryptionType::None;
}

} // namespace LiveKitCpp
