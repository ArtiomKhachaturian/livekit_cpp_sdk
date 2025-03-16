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
#elif defined(_WIN32)
#include <Windows.h>
#endif // __APPLE__
#include <string>
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

enum class TransportState;
enum class NetworkType;

#ifdef __APPLE__
std::string fromNSString(NSString* nsString);
NSString* toNSString(std::string_view string);
std::string toString(NSError* error);
// timestamps
// return zero if failed
#ifdef WEBRTC_MAC
int64_t cmTimeToMicro(const CMTime& time);
int32_t cmTimeToMilli(const CMTime& time);
#endif
#elif defined(_WIN32)
// https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types
std::vector<BYTE> queryRegistryValue(HKEY root, LPCSTR lpSubKey, LPCSTR lpValueName = NULL, LPDWORD lpType = NULL);
#endif // __APPLE__
std::string operatingSystemVersion();
std::string operatingSystemName();
std::string modelIdentifier();
// wifi, wired, cellular, vpn, empty if not known
NetworkType activeNetworkType();
std::string fromWideChar(const std::wstring& w);

template <unsigned flag>
inline constexpr bool testFlag(unsigned flags) { return flag == (flag & flags); }

inline constexpr uint64_t clueToUint64(int32_t i32hw, int32_t i32lw) { return (uint64_t(i32hw) << 32) | i32lw; }
inline constexpr int32_t extractHiWord(uint64_t i64) { return int32_t(i64 >> 32); }
inline constexpr int32_t extractLoWord(uint64_t i64) { return int32_t(i64 & 0xffffffffUL); }

std::string makeStateChangesString(TransportState from, TransportState to);

#ifdef WEBRTC_AVAILABLE
std::string fourccToString(int fourcc);
std::string makeUuid();
// human readable string for reflect of changes for some types
std::string makeStateChangesString(webrtc::PeerConnectionInterface::PeerConnectionState from,
                                   webrtc::PeerConnectionInterface::PeerConnectionState to);
std::string makeStateChangesString(webrtc::PeerConnectionInterface::IceConnectionState from,
                                   webrtc::PeerConnectionInterface::IceConnectionState to);
std::string makeStateChangesString(webrtc::PeerConnectionInterface::SignalingState from,
                                   webrtc::PeerConnectionInterface::SignalingState to);
std::string makeStateChangesString(webrtc::PeerConnectionInterface::IceGatheringState from,
                                   webrtc::PeerConnectionInterface::IceGatheringState to);
std::string makeStateChangesString(webrtc::TaskQueueBase::DelayPrecision from,
                                   webrtc::TaskQueueBase::DelayPrecision to);
// task queue helpers
std::shared_ptr<webrtc::TaskQueueBase> createTaskQueue(webrtc::TaskQueueFactory::Priority priority,
                                                       absl::string_view queueName = {},
                                                       const webrtc::FieldTrialsView* fieldTrials = nullptr);
#endif

} // namespace LiveKitCpp
