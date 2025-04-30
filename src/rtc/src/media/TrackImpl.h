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
#include "Listeners.h"
#include "StatsSourceImpl.h"
#include "TrackManager.h"
#include "Utils.h"
#include "livekit/rtc/media/MediaDevice.h"
#include "livekit/rtc/media/MediaEventsListener.h"
#include "livekit/rtc/media/NetworkPriority.h"
#include <atomic>

namespace LiveKitCpp
{

template <class TMediaDevice, class TTrackApi>
class TrackImpl : public TTrackApi, protected MediaEventsListener
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
    NetworkPriority networkPriority() const final { return _networkPriority; }
    void setNetworkPriority(NetworkPriority priority) final;
    double bitratePriority() const final { return _bitratePriority; }
    void setBitratePriority(double priority) final;
protected:
    TrackImpl(std::shared_ptr<TMediaDevice> mediaDevice, const std::weak_ptr<TrackManager>& trackManager);
    const auto& mediaDevice() const noexcept { return _mediaDevice; }
    std::shared_ptr<TrackManager> trackManager() const noexcept { return _trackManager.lock(); }
    webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback> statsCollector() const;
    virtual void onMuteChanged(bool /*mute*/) const {}
    virtual void onNetworkPriorityChanged(NetworkPriority /*priority*/) {}
    virtual void onBitratePriorityChanged(double /*priority*/) {}
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    static bool setBitratePriority(double priority, std::vector<webrtc::RtpEncodingParameters>& encodings);
    static bool setBitratePriority(double priority, webrtc::RtpParameters& parameters);
    static bool setNetworkPriority(NetworkPriority priority, std::vector<webrtc::RtpEncodingParameters>& encodings);
    static bool setNetworkPriority(NetworkPriority priority, webrtc::RtpParameters& parameters);
private:
    // impl. of MediaDeviceListener
    void onMuteChanged(const std::string&, bool mute) final;
private:
    const webrtc::scoped_refptr<StatsSourceImpl> _statsCollector;
    const std::shared_ptr<TMediaDevice> _mediaDevice;
    const std::weak_ptr<TrackManager> _trackManager;
    Bricks::Listeners<MediaEventsListener*> _listeners;
    std::atomic<NetworkPriority> _networkPriority = NetworkPriority::Low;
    std::atomic<double> _bitratePriority = webrtc::kDefaultBitratePriority;
};

template <class TMediaDevice, class TTrackApi>
inline TrackImpl<TMediaDevice, TTrackApi>::TrackImpl(std::shared_ptr<TMediaDevice> mediaDevice,
                                                     const std::weak_ptr<TrackManager>& trackManager)
    : _statsCollector(webrtc::make_ref_counted<StatsSourceImpl>())
    , _mediaDevice(std::move(mediaDevice))
    , _trackManager(trackManager)
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

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::setNetworkPriority(NetworkPriority priority)
{
    if (exchangeVal(priority, _networkPriority)) {
        onNetworkPriorityChanged(priority);
    }
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::setBitratePriority(double priority)
{
    if (exchangeVal(priority, _bitratePriority)) {
        onBitratePriorityChanged(priority);
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
inline bool TrackImpl<TMediaDevice, TTrackApi>::setBitratePriority(double priority,
                                                                   std::vector<webrtc::RtpEncodingParameters>& encodings)
{
    bool changed = false;
    if (!encodings.empty()) {
        for (auto& encoding : encodings) {
            if (!floatsIsEqual(encoding.bitrate_priority, priority)) {
                encoding.bitrate_priority = priority;
                changed = true;
            }
        }
    }
    return changed;
}

template <class TMediaDevice, class TTrackApi>
inline bool TrackImpl<TMediaDevice, TTrackApi>::setBitratePriority(double priority,
                                                                   webrtc::RtpParameters& parameters)
{
    return setBitratePriority(priority, parameters.encodings);
}

template <class TMediaDevice, class TTrackApi>
inline bool TrackImpl<TMediaDevice, TTrackApi>::setNetworkPriority(NetworkPriority priority,
                                                                   std::vector<webrtc::RtpEncodingParameters>& encodings)
{
    bool changed = false;
    if (!encodings.empty()) {
        webrtc::Priority rtcPriority = webrtc::Priority::kLow;
        switch (priority) {
            case NetworkPriority::VeryLow:
                rtcPriority = webrtc::Priority::kVeryLow;
                break;
            case NetworkPriority::Low:
                break;
            case NetworkPriority::Medium:
                rtcPriority = webrtc::Priority::kMedium;
                break;
            case NetworkPriority::High:
                rtcPriority = webrtc::Priority::kHigh;
                break;
            default:
                assert(false);
                break;
        }
        for (auto& encoding : encodings) {
            if (encoding.network_priority != rtcPriority) {
                encoding.network_priority = rtcPriority;
                changed = true;
            }
        }
    }
    return changed;
}

template <class TMediaDevice, class TTrackApi>
inline bool TrackImpl<TMediaDevice, TTrackApi>::setNetworkPriority(NetworkPriority priority,
                                                                   webrtc::RtpParameters& parameters)
{
    return setNetworkPriority(priority, parameters.encodings);
}

template <class TMediaDevice, class TTrackApi>
inline void TrackImpl<TMediaDevice, TTrackApi>::onMuteChanged(const std::string&, bool mute)
{
    onMuteChanged(mute);
}

} // namespace LiveKitCpp
