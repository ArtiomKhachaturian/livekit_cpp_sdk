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
#include <string>
#include <system_error>

#ifdef __APPLE__
#ifdef __OBJC__
@class NSString;
#else
typedef struct objc_object NSString;
#endif
#endif // WEBRTC_MAC

namespace LiveKitCpp
{

std::string NSStringToStdString(NSString* nsString);
std::string operatingSystemVersion();
std::string operatingSystemName();
std::string modelIdentifier();
std::string fromWideChar(const std::wstring& w);
std::string toString(const std::system_error& e);

template <unsigned flag>
inline constexpr bool testFlag(unsigned flags) { return flag == (flag & flags); }

} // namespace LiveKitCpp
