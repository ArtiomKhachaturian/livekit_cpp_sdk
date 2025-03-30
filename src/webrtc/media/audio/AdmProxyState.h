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
#pragma once // AdmProxyState.h
#include "AdmProxyTypedefs.h"
#include "AsyncListeners.h"
#include "MediaDeviceInfo.h"
#include "SafeObjAliases.h"
#include <atomic>
#include <optional>

namespace LiveKitCpp
{

class AdmProxyListener;

class AdmProxyState
{
    using AdmT = webrtc::AudioDeviceModule;
public:
    AdmProxyState(bool recording, std::weak_ptr<webrtc::TaskQueueBase> queue);
    void clearListeners() { _listeners.clear(); }
    void registereListener(AdmProxyListener* listener, bool reg);
    void update(const AdmPtr& adm);
    bool setCurrentDevice(const MediaDeviceInfo& info, const AdmPtr& adm = {});
    bool setStarted(bool started = true);
    bool setStopped() { return setStarted(false); }
    bool setMute(bool mute);
    bool setMute(const AdmPtr& adm);
    bool setStereo(bool stereo);
    bool setStereo(const AdmPtr& adm);
    bool setMinMaxVolume(uint32_t minVolume, uint32_t maxVolume);
    bool setMinMaxVolume(const AdmPtr& adm);
    bool setMinVolume(uint32_t minVolume);
    bool setMinVolume(const AdmPtr& adm);
    bool setMaxVolume(uint32_t maxVolume);
    bool setMaxVolume(const AdmPtr& adm);
    bool setVolume(uint32_t volume);
    bool setVolume(const AdmPtr& adm);
    MediaDeviceInfo currentDevice() const { return _dev(); }
    bool started() const noexcept { return _started; }
    bool mute() const noexcept { return _mute; }
    bool stereo() const noexcept { return _stereo; }
    uint32_t minVolume() const;
    uint32_t maxVolume() const;
    uint32_t volume() const noexcept { return _volume; }
private:
    template<class Method, typename... Args>
    void invoke(Method method, Args&&... args) const;
    template<typename T, class Getter>
    static std::optional<T> admProperty(const AdmPtr& adm, const Getter& getter);
private:
    const bool _recording;
    AsyncListeners<AdmProxyListener*, true> _listeners;
    Bricks::SafeObj<MediaDeviceInfo> _dev;
    std::atomic_bool _started = false;
    std::atomic_bool _mute = false;
    std::atomic_bool _stereo = false;
    std::atomic<uint64_t> _minMaxVolume = 0U;
    std::atomic<uint32_t> _volume = 0U;
};

} // namespace LiveKitCpp
