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
#pragma once // AdmRecordingListener.h
#include <cstdint>
#include <optional>

namespace LiveKitCpp
{

struct MediaDeviceInfo;

class AdmRecordingListener
{
public:
    virtual void onRecordingVolumeChanged(uint32_t /*volume*/) {}
    virtual void onRecordingMuteChanged(bool /*mute*/) {}
    virtual void onRecordingInitialized(const MediaDeviceInfo& /*info*/,
                                        const std::optional<bool>& /*stereo*/,
                                        const std::optional<uint32_t>& /*minVolume*/,
                                        const std::optional<uint32_t>& /*maxVolume*/,
                                        const std::optional<uint32_t>& /*currentVolume*/) {}
    virtual void onRecordingStarted() {}
    virtual void onRecordingStopped() {}
    virtual void onRecordingChanged(const MediaDeviceInfo& /*info*/,
                                    const std::optional<bool>& /*stereo*/,
                                    const std::optional<uint32_t>& /*minVolume*/,
                                    const std::optional<uint32_t>& /*maxVolume*/,
                                    const std::optional<uint32_t>& /*currentVolume*/) {}
    virtual void onRecordingStereoChanged(bool /*stereo*/) {}
protected:
    virtual ~AdmRecordingListener() = default;
};

} // namespace LiveKitCpp
