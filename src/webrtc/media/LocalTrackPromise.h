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
#include "LocalTrackFactory.h"
#include <api/scoped_refptr.h>
#include <api/media_stream_interface.h>
#include <api/rtp_sender_interface.h>
#include <atomic>
#include <string>

namespace LiveKitCpp
{

template<class TMediaTrackInterface>
class LocalTrackPromise
{
    enum State {
        None        = 0x00,
        Requested   = 0x01,
        Enabled     = 0x02
    };
public:
    LocalTrackPromise(std::string label, LocalTrackFactory* factory);
    void setEnabled(bool enable);
    bool enabled() const noexcept { return _enabled; }
    bool setRequestedSender(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender);
    void notifyAboutRequestFailure(const std::string& trackLabel);
protected:
    const auto& label() const noexcept { return _label; }
    LocalTrackFactory* factory() const noexcept { return _factory; }
    virtual webrtc::scoped_refptr<TMediaTrackInterface> createMediaTrack() = 0;
private:
    void changeEnabled(bool enabled, bool notify = true) const;
    bool accept(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const;
    bool accept(const std::string& label) const;
private:
    const std::string _label;
    LocalTrackFactory* const _factory;
    std::atomic_bool _enabled = false;
    SafeScopedRefPtr<webrtc::RtpSenderInterface> _sender;
    bool _requested = false; // united protection with [_sender]
};

template<class TMediaTrackInterface>
LocalTrackPromise<TMediaTrackInterface>::LocalTrackPromise(std::string label, LocalTrackFactory* factory)
    : _label(std::move(label))
    , _factory(factory)
{
}

template<class TMediaTrackInterface>
inline void LocalTrackPromise<TMediaTrackInterface>::setEnabled(bool enabled)
{
    if (enabled != _enabled.exchange(enabled)) {
        LOCK_WRITE_SAFE_OBJ(_sender);
        if (enabled) {
            if (_sender.constRef()) {
                changeEnabled(true);
            }
            else if (!_requested && factory()) {
                if (auto track = createMediaTrack()) {
                    _requested = factory()->add(std::move(track));
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
inline bool LocalTrackPromise<TMediaTrackInterface>::
    setRequestedSender(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender)
{
    LOCK_WRITE_SAFE_OBJ(_sender);
    if (_requested && accept(sender)) {
        _sender = std::move(sender);
        _requested = false;
        changeEnabled(enabled(), false);
        return true;
    }
    return false;
}

template<class TMediaTrackInterface>
inline void LocalTrackPromise<TMediaTrackInterface>::notifyAboutRequestFailure(const std::string& trackLabel)
{
    LOCK_WRITE_SAFE_OBJ(_sender);
    if (_requested && accept(trackLabel)) {
        _requested = false;
    }
}

template<class TMediaTrackInterface>
inline void LocalTrackPromise<TMediaTrackInterface>::changeEnabled(bool enabled, bool notify) const
{
    if (const auto& sender = _sender.constRef()) {
        if (const auto track = sender->track()) {
            const auto wasEnabled = track->enabled();
            if (wasEnabled != enabled) {
                track->set_enabled(enabled);
                if (notify) {
                    
                }
            }
        }
    }
}

template<class TMediaTrackInterface>
inline bool LocalTrackPromise<TMediaTrackInterface>::accept(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender) const
{
    if (sender) {
        if (const auto track = sender->track()) {
            return accept(track->id());
        }
    }
    return false;
}

template<class TMediaTrackInterface>
inline bool LocalTrackPromise<TMediaTrackInterface>::accept(const std::string& label) const
{
    return !label.empty() && label == this->label();
}

} // namespace LiveKitCpp
