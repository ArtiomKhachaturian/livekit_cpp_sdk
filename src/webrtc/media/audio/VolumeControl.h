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
#pragma once // VolumeControl.h
#include <cstdint>

namespace LiveKitCpp
{

class VolumeControl
{
public:
    VolumeControl() = default;
    VolumeControl(uint32_t minVolume, uint32_t maxVolume);
    void reset() { _minVolume = _maxVolume = 0U; }
    bool setRange(uint32_t minVolume, uint32_t maxVolume);
    bool valid() const;
    bool setNormalizedVolume(double volume);
    bool setVolume(uint32_t volume);
    uint32_t volume() const;
    double normalizedVolume() const noexcept { return _normalizedVolume; }
    explicit operator bool() const noexcept { return valid(); }
private:
    uint32_t _minVolume = 0U;
    uint32_t _maxVolume = 0U;
    double _normalizedVolume = 0.;
};

} // namespace LiveKitCpp
