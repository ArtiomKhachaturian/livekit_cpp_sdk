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
#pragma once // AdmProxyListener.h
#include <cstdint>

namespace LiveKitCpp
{

struct MediaDeviceInfo;

class AdmProxyListener
{
public:
    virtual void onStarted(bool /*recording*/) {}
    virtual void onStopped(bool /*recording*/) {}
    virtual void onMuteChanged(bool /*recording*/, bool /*mute*/) {}
    virtual void onStereoChanged(bool /*recording*/, bool /*stereo*/) {}
    virtual void onMinMaxVolumeChanged(bool /*recording*/, uint32_t /*minVolume*/,
                                       uint32_t /*maxVolume*/) {}
    virtual void onVolumeChanged(bool /*recording*/, uint32_t /*volume*/) {}
    virtual void onDeviceChanged(bool /*recording*/, const MediaDeviceInfo& /*info*/) {}
protected:
    virtual ~AdmProxyListener() = default;
};

} // namespace LiveKitCpp
