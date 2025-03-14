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
#ifdef WEBRTC_AVAILABLE
#include <api/peer_connection_interface.h>
#include <api/task_queue/task_queue_base.h>
#include <memory>
#endif
#ifdef __APPLE__
#include <CoreMedia/CMTime.h>
#endif // __APPLE__
#include <string>

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

enum class TransportState;
enum class NetworkType;

#ifdef __APPLE__
std::string fromNSString(NSString* nsString);
NSString* toNSString(std::string_view string);
std::string toString(NSError* error);
#endif // __APPLE__
std::string operatingSystemVersion();
std::string operatingSystemName();
std::string modelIdentifier();
// wifi, wired, cellular, vpn, empty if not known
NetworkType activeNetworkType();
std::string fromWideChar(const std::wstring& w);

std::string makeStateChangesString(TransportState from, TransportState to);

} // namespace LiveKitCpp
