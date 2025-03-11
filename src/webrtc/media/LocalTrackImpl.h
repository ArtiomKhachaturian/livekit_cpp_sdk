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
#pragma once // LocalTrackPromise.h
#include "SafeScopedRefPtr.h"
#include "LocalTrackManager.h"
#include "LocalTrack.h"
#include "Utils.h"
#include "rtc/AddTrackRequest.h"
#include <api/scoped_refptr.h>
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>
#include <atomic>
#include <string>

namespace LiveKitCpp
{

enum class SetSenderResult
{
    Accepted,
    Rejected,
    NotMatchedToRequest
};

template<class TMediaTrackInterface>
class LocalTrackImpl : public LocalTrack
{
public:
    LocalTrackImpl(std::string label, LocalTrackManager* manager);
    void setEnabled(bool enable);
    void setSid(const std::string& sid) { _sid(sid); }
    SetSenderResult setRequested(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    void notifyAboutRequestFailure(const std::string& id);
    // impl. of LocalTrack
    std::string cid() const final { return _cid; }
    std::string sid() const final { return _sid(); }
    bool enabled() const noexcept final { return _enabled; }
    bool fillRequest(AddTrackRequest& request) const override;
protected:
    LocalTrackManager* manager() const noexcept { return _manager; }
    virtual webrtc::scoped_refptr<TMediaTrackInterface> createMediaTrack(const std::string& id) = 0;
private:
    void changeEnabled(bool enabled, bool notify = true) const;
    bool accept(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const;
    bool accept(const std::string& id) const { return !id.empty() && id == _cid; }
private:
    const std::string _cid;
    const std::string _label;
    LocalTrackManager* const _manager;
    Bricks::SafeObj<std::string> _sid;
    std::atomic_bool _enabled = false;
    SafeScopedRefPtr<webrtc::RtpSenderInterface> _sender;
    bool _requested = false; // united protection with [_sender]
};

template<class TMediaTrackInterface>
LocalTrackImpl<TMediaTrackInterface>::LocalTrackImpl(std::string label,
                                                     LocalTrackManager* manager)
    : _cid(makeUuid())
    , _label(std::move(label))
    , _manager(manager)
{
}

template<class TMediaTrackInterface>
inline void LocalTrackImpl<TMediaTrackInterface>::setEnabled(bool enabled)
{
    if (enabled != _enabled.exchange(enabled)) {
        LOCK_WRITE_SAFE_OBJ(_sender);
        if (enabled) {
            if (_sender.constRef()) {
                changeEnabled(true);
            }
            else if (!_requested && _manager) {
                if (auto track = createMediaTrack(_cid)) {
                    _requested = _manager->add(std::move(track));
                }
            }
        }
        else {
            _requested = false;
            changeEnabled(false);
        }
    }
}

template<class TMediaTrackInterface>
inline SetSenderResult LocalTrackImpl<TMediaTrackInterface>::
    setRequested(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender)
{
    LOCK_WRITE_SAFE_OBJ(_sender);
    if (_requested && sender) {
        if (accept(sender)) {
            _sender = sender;
            _requested = false;
            changeEnabled(enabled(), false);
            return SetSenderResult::Accepted;
        }
        return SetSenderResult::NotMatchedToRequest;
    }
    return SetSenderResult::Rejected;
}

template<class TMediaTrackInterface>
inline void LocalTrackImpl<TMediaTrackInterface>::notifyAboutRequestFailure(const std::string& id)
{
    LOCK_WRITE_SAFE_OBJ(_sender);
    if (_requested && accept(id)) {
        _requested = false;
    }
}

template<class TMediaTrackInterface>
inline bool LocalTrackImpl<TMediaTrackInterface>::fillRequest(AddTrackRequest& request) const
{
    LOCK_READ_SAFE_OBJ(_sender);
    if (_sender.constRef()) {
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
        request._name = _label;
        request._muted = muted();
        request._sid = sid();
        return true;
    }
    return false;
}

template<class TMediaTrackInterface>
inline void LocalTrackImpl<TMediaTrackInterface>::changeEnabled(bool enabled, bool notify) const
{
    if (const auto& sender = _sender.constRef()) {
        if (const auto track = sender->track()) {
            const auto wasEnabled = track->enabled();
            if (wasEnabled != enabled) {
                track->set_enabled(enabled);
                if (notify && _manager) {
                    _manager->notifyAboutEnabledChanges(*this);
                }
            }
        }
    }
}

template<class TMediaTrackInterface>
inline bool LocalTrackImpl<TMediaTrackInterface>::accept(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (sender && sender->media_type() == mediaType()) {
        if (const auto track = sender->track()) {
            return accept(track->id());
        }
    }
    return false;
}

} // namespace LiveKitCpp
