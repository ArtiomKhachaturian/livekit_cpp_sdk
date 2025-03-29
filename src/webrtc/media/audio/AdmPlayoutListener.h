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
#pragma once // AdmPlayoutListener.h
#include <cstdint>
#include <optional>

namespace LiveKitCpp
{

struct MediaDeviceInfo;

class AdmPlayoutListener
{
public:
    virtual void onPlayoutVolumeChanged(uint32_t /*volume*/) {}
    virtual void onPlayoutMuteChanged(bool /*mute*/) {}
    virtual void onPlayoutInitialized(const MediaDeviceInfo& /*info*/,
                                      const std::optional<bool>& /*stereo*/,
                                      const std::optional<uint32_t>& /*minVolume*/,
                                      const std::optional<uint32_t>& /*maxVolume*/,
                                      const std::optional<uint32_t>& /*currentVolume*/) {}
    virtual void onPlayoutStarted() {}
    virtual void onPlayoutStopped() {}
    virtual void onPlayoutChanged(const MediaDeviceInfo& /*info*/,
                                  const std::optional<bool>& /*stereo*/,
                                  const std::optional<uint32_t>& /*minVolume*/,
                                  const std::optional<uint32_t>& /*maxVolume*/,
                                  const std::optional<uint32_t>& /*currentVolume*/) {}
    virtual void onPlayoutStereoChanged(bool /*stereo*/) {}
protected:
    virtual ~AdmPlayoutListener() = default;
};

} // namespace LiveKitCpp
