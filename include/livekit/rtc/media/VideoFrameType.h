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
#pragma once // VideoFrameType.h
#include "livekit/rtc/LiveKitRtcExport.h"
#include <string>

namespace LiveKitCpp
{

enum class VideoFrameType
{
    // non-planar (single plane) types (RGBX)
    RGB24, // RGB big endian (rgb in memory)
    BGR24, // RGB little endian (bgr in memory)
    BGRA32, // ARGB little endian (bgra in memory)
    ARGB32, // BGRA little endian (argb in memory)
    RGBA32, // ABGR little endian (rgba in memory)
    ABGR32, // RGBA little endian (abgr in memory)
    RGB565, // the image is stored using a premultiplied 24-bit ARGB format (8-5-6-5)
    MJPEG,
    UYVY,
    YUY2,
    // planar types
    NV12,
    I420,
    I422,
    I444,
    I010,
    I210,
    I410,
    YV12,
    IYUV, // similar to I420
};

LIVEKIT_RTC_API std::string toString(VideoFrameType type);
LIVEKIT_RTC_API bool isRGB(VideoFrameType type);

} // namespace LiveKitCpp
