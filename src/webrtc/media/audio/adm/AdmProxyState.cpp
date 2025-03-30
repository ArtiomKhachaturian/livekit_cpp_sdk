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
#include "AdmProxyState.h"
#include "AdmProxyListener.h"
#include "VolumeControl.h"
#include "Utils.h"
#include <modules/audio_device/include/audio_device.h> //AudioDeviceModule

namespace LiveKitCpp
{

AdmProxyState::AdmProxyState(bool recording, std::weak_ptr<webrtc::TaskQueueBase> queue)
    : _recording(recording)
    , _listeners(queue)
{
}

void AdmProxyState::registereListener(AdmProxyListener* listener, bool reg)
{
    if (reg) {
        _listeners.add(listener);
    }
    else {
        _listeners.remove(listener);
    }
}

bool AdmProxyState::registered(AdmProxyListener* listener) const
{
    return _listeners.contains(listener);
}

void AdmProxyState::update(const AdmPtr& adm)
{
    setMute(adm);
    setStereo(adm);
    setMinMaxVolume(adm);
    setVolume(adm);
}

bool AdmProxyState::setCurrentDevice(const MediaDeviceInfo& info, const AdmPtr& adm)
{
    if (exchangeVal(info, _dev)) {
        update(adm);
        invoke(&AdmProxyListener::onDeviceChanged, info);
        return true;
    }
    return false;
}

bool AdmProxyState::setStarted(bool started)
{
    if (exchangeVal(started, _started)) {
        if (started) {
            invoke(&AdmProxyListener::onStarted);
        }
        else {
            invoke(&AdmProxyListener::onStopped);
        }
    }
}

bool AdmProxyState::setMute(bool mute)
{
    if (exchangeVal(mute, _mute)) {
        invoke(&AdmProxyListener::onMuteChanged, mute);
        return true;
    }
    return false;
}

bool AdmProxyState::setMute(const AdmPtr& adm)
{
    if (adm) {
        const auto method = _recording ? &AdmT::MicrophoneMute : &AdmT::SpeakerMute;
        if (const auto mute = admProperty<bool>(adm, method)) {
            return setMute(mute.value());
        }
    }
    return false;
}

bool AdmProxyState::setStereo(bool stereo)
{
    if (exchangeVal(stereo, _stereo)) {
        invoke(&AdmProxyListener::onStereoChanged, stereo);
        return true;
    }
    return false;
}

bool AdmProxyState::setStereo(const AdmPtr& adm)
{
    if (adm) {
        const auto method = _recording ? &AdmT::StereoRecording : &AdmT::StereoPlayout;
        if (const auto stereo = admProperty<bool>(adm, method)) {
            return setStereo(stereo.value());
        }
    }
    return false;
}

bool AdmProxyState::setMinMaxVolume(uint32_t minVolume, uint32_t maxVolume)
{
    const auto common = clueToUint64(minVolume, maxVolume);
    if (exchangeVal(common, _minMaxVolume)) {
        invoke(&AdmProxyListener::onMinMaxVolumeChanged, minVolume, maxVolume);
        return true;
    }
    return false;
}

bool AdmProxyState::setMinMaxVolume(const AdmPtr& adm)
{
    if (adm) {
        const auto minM = _recording ? &AdmT::MinMicrophoneVolume : &AdmT::MinSpeakerVolume;
        if (const auto minV = admProperty<uint32_t>(adm, minM)) {
            const auto maxM = _recording ? &AdmT::MaxMicrophoneVolume : &AdmT::MaxSpeakerVolume;
            if (const auto maxV = admProperty<uint32_t>(adm, maxM)) {
                return setMinMaxVolume(minV.value(), maxV.value());
            }
        }
    }
    return false;
}

bool AdmProxyState::setMinVolume(uint32_t minVolume)
{
    return setMinMaxVolume(minVolume, maxVolume());
}

bool AdmProxyState::setMinVolume(const AdmPtr& adm)
{
    if (adm) {
        const auto method = _recording ? &AdmT::MinMicrophoneVolume : &AdmT::MinSpeakerVolume;
        if (const auto volume = admProperty<uint32_t>(adm, method)) {
            return setMinVolume(volume.value());
        }
    }
    return false;
}

bool AdmProxyState::setMaxVolume(uint32_t maxVolume)
{
    return setMinMaxVolume(minVolume(), maxVolume);
}

bool AdmProxyState::setMaxVolume(const AdmPtr& adm)
{
    if (adm) {
        const auto method = _recording ? &AdmT::MaxMicrophoneVolume : &AdmT::MaxSpeakerVolume;
        if (const auto volume = admProperty<uint32_t>(adm, method)) {
            return setMaxVolume(volume.value());
        }
    }
    return false;
}

bool AdmProxyState::setVolume(uint32_t volume)
{
    if (exchangeVal(volume, _volume)) {
        invoke(&AdmProxyListener::onVolumeChanged, volume);
        return true;
    }
    return false;
}

bool AdmProxyState::setVolume(const AdmPtr& adm)
{
    if (adm) {
        const auto method = _recording ? &AdmT::MicrophoneVolume : &AdmT::SpeakerVolume;
        if (const auto volume = admProperty<uint32_t>(adm, method)) {
            return setVolume(volume.value());
        }
    }
    return false;
}

uint32_t AdmProxyState::minVolume() const
{
    return extractHiWord(_minMaxVolume);
}

uint32_t AdmProxyState::maxVolume() const
{
    return extractLoWord(_minMaxVolume);
}

template<class Method, typename... Args>
void AdmProxyState::invoke(Method method, Args&&... args) const
{
    if (_listeners) {
        _listeners.invoke(std::move(method), _recording, std::forward<Args>(args)...);
    }
}

template<typename T, class Getter>
std::optional<T> AdmProxyState::admProperty(const AdmPtr& adm, const Getter& getter)
{
    if (adm) {
        T val;
        if (0 == ((*adm).*getter)(&val)) {
            return val;
        }
    }
    return {};
}

} // namespace LiveKitCpp
