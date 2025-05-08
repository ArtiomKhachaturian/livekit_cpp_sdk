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
#pragma once // Utils.h
#include "SafeObj.h"
#include <memory>
#ifdef __APPLE__
#include <CoreMedia/CMTime.h>
#include <CoreGraphics/CGDirectDisplay.h>
#elif defined(_WIN32)
#include <Windows.h>
#endif // __APPLE__
#include <algorithm>
#include <atomic>
#include <cmath>
#include <optional>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>

#ifdef __APPLE__
#ifdef __OBJC__
@class NSString;
@class NSError;
#else
typedef struct objc_object NSString;
typedef struct objc_object NSError;
#endif
#endif // __APPLE__

namespace LiveKitCpp
{

#ifdef __APPLE__
std::string fromNSString(NSString* nsString);
NSString* toNSString(std::string_view string);
NSString* toNSString(CFStringRef string);
std::string toString(NSError* error);
// timestamps
// return zero if failed
bool compareCFStrings(CFStringRef s1, CFStringRef s2, bool caseInsensitive);
std::string stringFromCFString(CFStringRef str);
#elif defined(_WIN32)
// https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types
std::vector<BYTE> queryRegistryValue(HKEY root, LPCSTR lpSubKey, LPCSTR lpValueName = NULL, LPDWORD lpType = NULL);
HRESULT initializeComForThisThread(bool multiThreadedModel = true);
#endif // __APPLE__

// major version / major version / patch version
std::tuple<int, int, int> operatingSystemVersion();
std::string operatingSystemVersionString(bool withPatchVersion = false);
std::string operatingSystemName();

// string utilities
// emulation of non-standard [::strcmpi] function, return true if both strings are identical
bool compareCaseInsensitive(std::string_view s1, std::string_view s2);
inline bool compareCaseSensitive(std::string_view s1, std::string_view s2) {
    return 0 == s1.compare(s2);
}

std::string toLower(std::string s);
std::string fromWideChar(std::wstring_view w);
std::vector<std::string> split(std::string_view s, std::string_view delim);
std::string join(const std::vector<std::string>& strings,
                 std::string_view delim, bool skipEmpty = false);
std::string join(const std::vector<std::string_view>& strings,
                 std::string_view delim, bool skipEmpty = false);
inline bool startWith(std::string_view string, std::string_view sv) {
    return  0U == string.rfind(sv, 0U);
}

template <unsigned flag>
inline constexpr bool testFlag(unsigned flags) { return flag == (flag & flags); }

inline constexpr uint64_t clueToUint64(int32_t i32hw, int32_t i32lw) { return (uint64_t(i32hw) << 32) | i32lw; }
inline constexpr int32_t extractHiWord(uint64_t i64) { return int32_t(i64 >> 32); }
inline constexpr int32_t extractLoWord(uint64_t i64) { return int32_t(i64 & 0xffffffffUL); }

template <typename T>
inline std::string toHexValue(T value) {
    std::ostringstream ss;
    ss << std::hex << value;
    return ss.str();
}

template <typename T>
std::optional<T> fromHexValue(const std::string& str) {
    if (!str.empty()) {
        std::istringstream ss(str);
        T value;
        ss >> std::hex >> value;
        if (!ss.fail()) {
            return value;
        }
    }
    return std::nullopt;
}

template <typename T>
inline bool exchangeVal(T source, Bricks::SafeObj<T>& dst) {
    const std::lock_guard guard(dst.mutex());
    if (source != dst.constRef()) {
        dst = std::move(source);
        return true;
    }
    return false;
}

template <typename T>
inline bool exchangeVal(const T& source, std::atomic<T>& dst) {
    return source != dst.exchange(source);
}

template <typename T> // this function is better than [std::clamp] (constexpr, no dangling references on output)
inline constexpr T bound(const T& min, const T& val, const T& max) {
    return std::max(min, std::min(max, val));
}

template <typename TFloat1, typename TFloat2>
inline bool floatsIsEqual(TFloat1 x, TFloat2 y)
{
    static_assert(std::is_arithmetic<TFloat1>::value && std::is_arithmetic<TFloat2>::value);
    bool const xnan = std::isnan(x), ynan = std::isnan(y);
    if (xnan || ynan) { // if one is nan -> return false, if both -> true
        return xnan && ynan;
    }
    // both values should not be greater or less of each other
    return !std::islessgreater(x, y);
}

template <typename T>
inline T even2(T num) { return (num + 1) & ~1; }

inline std::vector<uint8_t> binaryFromString(std::string_view s) { return {s.begin(), s.end()}; }

// hash support
template <typename T>
inline size_t hashCombine(const T& v) { return std::hash<T>()(v); }

template <typename T, typename... Rest>
inline size_t hashCombine(const T& v, Rest... rest) {
    size_t seed = hashCombine(rest...);
    seed ^= hashCombine<T>(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

template <class InputIt>
inline size_t hashCombineRange(InputIt first, InputIt last) {
    size_t seed = 0UL;
    for (; first != last; ++first) {
        seed = hashCombine(*first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

uint16_t checksumISO3309(const uint8_t* data, size_t len);

} // namespace LiveKitCpp
