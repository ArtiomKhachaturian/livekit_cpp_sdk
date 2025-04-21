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
#pragma once // VideoOptions.h
#include "livekit/rtc/LiveKitRtcExport.h"
#include "livekit/rtc/media/VideoFrameType.h"
#include <optional>
#include <cstdint>
#include <tuple>

namespace LiveKitCpp
{

struct LIVEKIT_RTC_API VideoOptions
{
    int32_t _width = 0;
    int32_t _height = 0;
    int32_t _maxFPS = 0;
    std::optional<VideoFrameType> _type;
    unsigned _flags = 0U;
    void setInterlaced(bool interlaced);
    void setPreview(bool interlaced);
    bool interlaced() const noexcept;
    bool preview() const noexcept;
    bool null() const noexcept { return 0 == _width && 0 == _height && 0 == _maxFPS; }
    explicit operator bool() const noexcept { return !null(); }
};

LIVEKIT_RTC_API std::string toString(const VideoOptions& options);

} // namespace LiveKitCpp

inline bool operator == (const LiveKitCpp::VideoOptions& l, const LiveKitCpp::VideoOptions& r) {
    return &l == &r || (std::tie(l._width, l._height, l._maxFPS, l._type, l._flags) ==
                        std::tie(r._width, r._height, r._maxFPS, r._type, r._flags));
}

inline bool operator != (const LiveKitCpp::VideoOptions& l, const LiveKitCpp::VideoOptions& r) {
    return std::tie(l._width, l._height, l._maxFPS, l._type, l._flags) !=
           std::tie(r._width, r._height, r._maxFPS, r._type, r._flags);
}

