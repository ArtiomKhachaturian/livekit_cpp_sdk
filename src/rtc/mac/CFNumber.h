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
#pragma once // CFNumber.h
#include "CFAutoRelease.h"
#include <CoreFoundation/CFNumber.h>
#include <MacTypes.h>
#include <type_traits>

namespace LiveKitCpp
{

using CFNumberRefAutoRelease = CFAutoRelease<CFNumberRef>;

template<typename T>
inline CFNumberType dedicateCFNumberType() {
    // unsigned types are not supported for CFNumber, convert it to signed
    static_assert(std::is_unsigned_v<T>, "sorry, but other types not supported, "
                                         "or specialization for inference of given type was not found");
    return dedicateCFNumberType<std::make_signed_t<T>>();
}
template<> inline constexpr CFNumberType dedicateCFNumberType<SInt8>() { return kCFNumberSInt8Type; }
template<> inline constexpr CFNumberType dedicateCFNumberType<SInt16>() { return kCFNumberSInt16Type; }
template<> inline constexpr CFNumberType dedicateCFNumberType<SInt32>() { return kCFNumberSInt32Type; }
template<> inline constexpr CFNumberType dedicateCFNumberType<SInt64>() { return kCFNumberSInt64Type; }
template<> inline constexpr CFNumberType dedicateCFNumberType<Float32>() { return kCFNumberFloat32Type; }
template<> inline constexpr CFNumberType dedicateCFNumberType<Float64>() { return kCFNumberFloat32Type; }
template<> inline constexpr CFNumberType dedicateCFNumberType<char>() { return kCFNumberCharType; }
template<typename T>
inline CFNumberRefAutoRelease createCFNumber(const T& value, CFNumberType type = dedicateCFNumberType<T>()) {
    return CFNumberCreate(kCFAllocatorDefault, type, &value);
}

} // namespace LiveKitCpp
