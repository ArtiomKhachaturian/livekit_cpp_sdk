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
class MediaDeviceImpl : public TBaseInterface
{
    static_assert(std::is_base_of_v<webrtc::MediaStreamTrackInterface, TTrack>);
    static_assert(std::is_base_of_v<MediaDevice, TBaseInterface>);
public:
    ~MediaDeviceImpl() override { _listeners.clear(); }
    // for manipulations with peer connection
    const auto& track() const noexcept { return _track; }
    // impl. of MediaDevice
    bool addListener(MediaEventsListener* listener) override;
    bool removeListener(MediaEventsListener* listener) override;
    bool live() const final;
    std::string id() const override { return _id; }
    void mute(bool mute) final;
    bool muted() const override { return _muted; }
protected:
    MediaDeviceImpl(webrtc::scoped_refptr<TTrack> track);
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
            _listeners.invoke(&MediaEventsListener::onMuteChanged, _track->id(), mute);
        }
    }
}

} // namespace LiveKitCpp
