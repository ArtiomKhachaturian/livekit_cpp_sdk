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
#pragma once // VideoLayer.h
#include "livekit/rtc/VideoQuality.h"
#include <cstdint>

namespace LiveKitCpp
{

// provide information about available spatial layers
struct VideoLayer
{
    // for tracks with a single layer, this should be HIGH
    VideoQuality _quality = {};
    uint32_t _width = {};
    uint32_t _height = {};
    // target bitrate in bit per second (bps), server will measure actual
    uint32_t _bitrate = {};
    uint32_t _ssrc = {};
};

} // namespace LiveKitCpp
