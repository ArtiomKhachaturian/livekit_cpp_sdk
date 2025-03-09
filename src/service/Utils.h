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
#endif
#include <string>

#ifdef __APPLE__
#ifdef __OBJC__
@class NSString;
#else
typedef struct objc_object NSString;
#endif
#endif // WEBRTC_MAC

namespace LiveKitCpp
{

enum class TransportState;
enum class NetworkType;

std::string NSStringToStdString(NSString* nsString);
std::string operatingSystemVersion();
std::string operatingSystemName();
std::string modelIdentifier();
// wifi, wired, cellular, vpn, empty if not known
NetworkType activeNetworkType();
std::string fromWideChar(const std::wstring& w);

template <unsigned flag>
inline constexpr bool testFlag(unsigned flags) { return flag == (flag & flags); }

std::string makeStateChangesString(TransportState from, TransportState to);

#ifdef WEBRTC_AVAILABLE
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
#endif

} // namespace LiveKitCpp
