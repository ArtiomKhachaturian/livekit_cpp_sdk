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
#pragma once // TrackImpl.h
#ifdef WEBRTC_AVAILABLE
#include "Listeners.h"
#include "StatsSourceImpl.h"
#include "TrackManager.h"
#include "media/MediaDevice.h"
#include "media/MediaEventsListener.h"
#include <atomic>

namespace LiveKitCpp
{

template <class TMediaDevice, class TTrackApi>
class TrackImpl : public TTrackApi, private MediaEventsListener
{
    static_assert(std::is_base_of_v<MediaDevice, TMediaDevice>);
    static_assert(std::is_base_of_v<Track, TTrackApi>);
public:
    ~TrackImpl() override;
    // impl. of StatsSource
    void addListener(StatsListener* listener) final;
    void removeListener(StatsListener* listener) final;
    // impl. of Track
    std::string id() const override;
    bool live() const final { return _mediaDevice && _mediaDevice->live(); }
    void mute(bool mute) final;
    bool muted() const override { return _mediaDevice && _mediaDevice->muted(); }
    void addListener(MediaEventsListener* listener) final;
    void removeListener(MediaEventsListener* listener) final;
protected:
    TrackImpl(std::shared_ptr<TMediaDevice> mediaDevice, TrackManager* manager);
    const auto& mediaDevice() const noexcept { return _mediaDevice; }
    TrackManager* manager() const noexcept { return _manager; }
    webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback> statsCollector() const;
    virtual void notifyAboutMuted(bool /*mute*/) const {}
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
private:
    // impl. of MediaDeviceListener
    void onMuteChanged(const std::string&, bool mute) final;
private:
    const webrtc::scoped_refptr<StatsSourceImpl> _statsCollector;
    const std::shared_ptr<TMediaDevice> _mediaDevice;
    TrackManager* const _manager;
    Bricks::Listeners<MediaEventsListener*> _listeners;
};

template <class TMediaDevice, class TTrackApi>
inline TrackImpl<TMediaDevice, TTrackApi>::TrackImpl(std::shared_ptr<TMediaDevice> mediaDevice,
                                                     TrackManager* manager)
    : _statsCollector(webrtc::make_ref_counted<StatsSourceImpl>())
    , _mediaDevice(std::move(mediaDevice))
    , _manager(manager)
{
    if (_mediaDevice) {
        _mediaDevice->addListener(this);
    }
}

template <class TMediaDevice, class TTrackApi>
inline TrackImpl<TMediaDevice, TTrackApi>::~TrackImpl()
{
    if (_mediaDevice) {
        _mediaDevice->removeListener(this);
    }
    _statsCollector->clearListeners();
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::addListener(StatsListener* listener)
{
    _statsCollector->addListener(listener);
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::removeListener(StatsListener* listener)
{
    _statsCollector->removeListener(listener);
}

template <class TMediaDevice, class TTrackApi>
inline std::string TrackImpl<TMediaDevice, TTrackApi>::id() const
{
    if (_mediaDevice) {
        return _mediaDevice->id();
    }
    return {};
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::mute(bool mute)
{
    if (_mediaDevice) {
        _mediaDevice->mute(mute);
    }
}

template <class TMediaDevice, class TTrackApi>
inline webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>
    TrackImpl<TMediaDevice, TTrackApi>::statsCollector() const
{
    return _statsCollector;
}

template <class TMediaDevice, class TTrackApi>
template <class Method, typename... Args>
inline void TrackImpl<TMediaDevice, TTrackApi>::notify(const Method& method, Args&&... args) const
{
    _listeners.invoke(method, std::forward<Args>(args)...);
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::onMuteChanged(const std::string&, bool mute)
{
    notifyAboutMuted(mute);
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::addListener(MediaEventsListener* listener)
{
    if (listener != this) {
        if (_mediaDevice) {
            _mediaDevice->addListener(listener);
        }
        _listeners.add(listener);
    }
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::removeListener(MediaEventsListener* listener)
{
    if (listener != this) {
        if (_mediaDevice) {
            _mediaDevice->removeListener(listener);
        }
        _listeners.remove(listener);
    }
}

} // namespace LiveKitCpp
#endif
