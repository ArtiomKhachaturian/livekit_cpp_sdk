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
#pragma once // MediaDeviceImpl.h
#include "Listeners.h"
#include "MediaDeviceListener.h"
#include "Utils.h"
#include "livekit/rtc/media/MediaDevice.h"
#include "livekit/rtc/media/MediaDevice.h"
#include "livekit/rtc/media/MediaEventsListener.h"
#include <api/media_stream_interface.h>
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

template <class TTrack, class TBaseInterface = MediaDevice>
class MediaDeviceImpl : public TBaseInterface, protected MediaDeviceListener
{
    static_assert(std::is_base_of_v<webrtc::MediaStreamTrackInterface, TTrack>);
    static_assert(std::is_base_of_v<MediaDevice, TBaseInterface>);
public:
    ~MediaDeviceImpl() override { _listeners.clear(); }
    // for manipulations with peer connection
    const auto& track() const noexcept { return _track; }
    // impl. of MediaDevice
    bool addListener(MediaEventsListener* listener) final;
    bool removeListener(MediaEventsListener* listener) final;
    bool live() const final;
    std::string id() const override { return _id; }
    void mute(bool mute) final;
    bool muted() const override { return _muted; }
protected:
    MediaDeviceImpl(webrtc::scoped_refptr<TTrack> track);
    // overrides of MediaDeviceListener
    void onMediaChanged() final;
    void onMediaCreationFailed(const std::string& details) final;
    void onMediaOptionsChanged() final;
    void onMediaStarted() final;
    void onMediaStartFailed(const std::string& details) final;
    void onMediaStopped() final;
    void onMediaStopFailed(const std::string& details) final;
    void onMediaFatalError(const std::string& details) final;
private:
    const webrtc::scoped_refptr<TTrack> _track;
    const std::string _id;
    std::atomic_bool _muted;
    Bricks::Listeners<MediaEventsListener*> _listeners;
};

template <class TTrack, class TBaseInterface>
inline MediaDeviceImpl<TTrack, TBaseInterface>::MediaDeviceImpl(webrtc::scoped_refptr<TTrack> track)
    : _track(std::move(track))
    , _id(_track ? _track->id() : std::string{})
    , _muted(!_track || !_track->enabled())
{
}

template <class TTrack, class TBaseInterface>
inline bool MediaDeviceImpl<TTrack, TBaseInterface>::addListener(MediaEventsListener* listener)
{
    return _track && Bricks::ok(_listeners.add(listener));
}

template <class TTrack, class TBaseInterface>
inline bool MediaDeviceImpl<TTrack, TBaseInterface>::removeListener(MediaEventsListener* listener)
{
    return _track && Bricks::ok(_listeners.remove(listener));
}

template <class TTrack, class TBaseInterface>
inline bool MediaDeviceImpl<TTrack, TBaseInterface>::live() const
{
    return _track && webrtc::MediaStreamTrackInterface::kLive == _track->state();
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::mute(bool mute)
{
    if (_track && exchangeVal(mute, _muted)) {
        const auto wasEnabled = _track->enabled();
        if (wasEnabled == mute) {
            _track->set_enabled(!mute);
            _listeners.invoke(&MediaEventsListener::onMuteChanged, _id, mute);
        }
    }
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaChanged()
{
    _listeners.invoke(&MediaEventsListener::onMediaChanged, _id);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaCreationFailed(const std::string& details)
{
    _listeners.invoke(&MediaEventsListener::onMediaCreationFailed, _id, details);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaOptionsChanged()
{
    _listeners.invoke(&MediaEventsListener::onMediaOptionsChanged, _id);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaStarted()
{
    _listeners.invoke(&MediaEventsListener::onMediaStarted, _id);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaStartFailed(const std::string& details)
{
    _listeners.invoke(&MediaEventsListener::onMediaStartFailed, _id, details);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaStopped()
{
    _listeners.invoke(&MediaEventsListener::onMediaStopped, _id);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaStopFailed(const std::string& details)
{
    _listeners.invoke(&MediaEventsListener::onMediaStopFailed, _id, details);
}

template <class TTrack, class TBaseInterface>
inline void MediaDeviceImpl<TTrack, TBaseInterface>::onMediaFatalError(const std::string& details)
{
    _listeners.invoke(&MediaEventsListener::onMediaFatalError, _id, details);
}

} // namespace LiveKitCpp
