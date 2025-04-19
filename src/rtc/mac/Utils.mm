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
#include "Utils.h"
#include <rtc_base/time_utils.h>
#include <api/units/timestamp.h>
#include <optional>
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

std::string fromNSString(NSString* nsString)
{
    if (nsString) {
        @autoreleasepool {
            NSData* charData = [nsString dataUsingEncoding:NSUTF8StringEncoding];
            if (charData) {
                return std::string(reinterpret_cast<const char *>(charData.bytes),
                                   charData.length);
            }
        }
    }
    return {};
}

NSString* toNSString(std::string_view string)
{
    if (!string.empty()) {
        // std::string_view may contain null termination character so we construct using length
        return [[NSString alloc] initWithBytes:string.data()
                                        length:string.length()
                                      encoding:NSUTF8StringEncoding];
    }
    return [NSString string]; // empty
}

std::string toString(NSError* error)
{
    if (error) {
        return fromNSString(error.localizedDescription);
    }
    return std::string();
}

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

bool compareCFStrings(CFStringRef s1, CFStringRef s2, bool caseInsensitive)
{
    if (s1 && s2) {
        CFStringCompareFlags flags = kCFCompareLocalized;
        if (caseInsensitive) {
            flags |= kCFCompareCaseInsensitive;
        }
        return kCFCompareEqualTo == CFStringCompare(s1, s2, flags);
    }
    return s1 == s2;
}

std::string stringFromCFString(CFStringRef str)
{
    if (str) {
        const auto size = CFStringGetLength(str);
        if (size > 0) {
            auto capacity = CFStringGetMaximumSizeForEncoding(size, kCFStringEncodingUTF8);
            if (capacity++ > 0) {
                static thread_local std::vector<char> raw;
                if (raw.size() < capacity) {
                    raw.resize(capacity);
                }
                if (CFStringGetCString(str, raw.data(), capacity, kCFStringEncodingUTF8)) {
                    return std::string(raw.data(), size);
                }
            }
        }
    }
    return std::string();
}

} // namespace LiveKitCpp

