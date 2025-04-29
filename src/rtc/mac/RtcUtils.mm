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
#include "RtcUtils.h"
#include <rtc_base/time_utils.h>
#include <api/units/timestamp.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

namespace {

inline std::optional<webrtc::Timestamp> toTimestamp(const CMTime& time)
{
    if (0 != CMTimeCompare(kCMTimeInvalid, time) && (time.flags & kCMTimeFlags_Valid)) {
        return webrtc::Timestamp::Seconds(CMTimeGetSeconds(time));
    }
    return std::nullopt;
}

}

namespace LiveKitCpp
{

int64_t cmTimeToMicro(const CMTime& time)
{
    const auto ts = toTimestamp(time);
    if (ts.has_value()) {
        return ts->us<int64_t>();
    }
    return 0LL;
}

int32_t cmTimeToMilli(const CMTime& time)
{
    const auto ts = toTimestamp(time);
    if (ts.has_value()) {
        return ts->ms<int32_t>();
    }
    return 0;
}

CFStringRefAutoRelease stringToCFString(std::string_view str)
{
    if (!str.empty()) {
        return CFStringCreateWithCString(kCFAllocatorDefault, str.data(), kCFStringEncodingUTF8);
    }
    return nullptr;
}

} // namespace LiveKitCpp

