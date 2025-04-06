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
#include "VolumeControl.h"
#include "Utils.h"

namespace {

template<typename T>
inline bool exchangeVolumeVal(const T& source, T& dst) {
    if (source != dst) {
        dst = source;
        return true;
    }
    return false;
}

}

namespace LiveKitCpp
{

VolumeControl::VolumeControl(uint32_t minVolume, uint32_t maxVolume)
{
    setRange(minVolume, maxVolume);
}

VolumeControl::VolumeControl(uint32_t volume, uint32_t minVolume, uint32_t maxVolume)
    : VolumeControl(minVolume, maxVolume)
{
    setVolume(volume);
}

VolumeControl::VolumeControl(double volume, uint32_t minVolume, uint32_t maxVolume)
    : VolumeControl(minVolume, maxVolume)
{
    setNormalizedVolume(volume);
}

bool VolumeControl::setRange(uint32_t minVolume, uint32_t maxVolume)
{
    const auto minv = std::min(minVolume, maxVolume);
    const auto maxv = std::max(minVolume, maxVolume);
    const auto b1 = exchangeVolumeVal(minv, _minVolume);
    const auto b2 = exchangeVolumeVal(maxv, _maxVolume);
    return b1 || b2;
}

bool VolumeControl::valid() const
{
    return (_maxVolume - _minVolume) > 0U;
}

bool VolumeControl::setNormalizedVolume(double volume)
{
    return exchangeVolumeVal(bound(0., volume, 1.), _normalizedVolume);
}

bool VolumeControl::setVolume(uint32_t volume)
{
    if (const auto range = _maxVolume - _minVolume) {
        volume = bound(_minVolume, volume, _maxVolume);
        const double normv = (volume - _minVolume) / double(range);
        return setNormalizedVolume(normv);
    }
    return false;
}

uint32_t VolumeControl::volume() const
{
    if (const auto range = _maxVolume - _minVolume) {
        return _minVolume + uint32_t(std::round(normalizedVolume() * range));
    }
    return 0U;
}

} // namespace LiveKitCpp
