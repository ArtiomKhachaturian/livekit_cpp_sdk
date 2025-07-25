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
#pragma once // IceTransportPolicy.h
#include "livekit/signaling/LiveKitSignalingExport.h"
#include <string>

namespace LiveKitCpp
{

/**
 * @enum IceTransportPolicy
 * @brief Defines the types of network transports that can be used for ICE (Interactive Connectivity Establishment) candidates in WebRTC.
 *
 * ICE (Interactive Connectivity Establishment) is a protocol that helps WebRTC find possible paths for connections using various network interfaces and NAT (Network Address Translation).
 *
 * @var IceTransportPolicy::kAll
 * Uses all available network interfaces (Wi-Fi, Ethernet, mobile networks, etc.) to discover candidates.
 * This option maximizes the chances of establishing a connection.
 *
 * @var IceTransportPolicy::kRelay
 * Uses only relay candidates such as STUN/TURN servers.
 * This option restricts connections to those that can be established through relay servers.
 *
 * @var IceTransportPolicy::kNoHost
 * Excludes local candidates.
 * This option is useful for preventing local IP address leaks.
 *
 * @var IceTransportPolicy::kNone
 * No candidates are used.
 * This option is typically not used, as it prevents the establishment of connections.
 */
enum class IceTransportPolicy
{
    None,   ///< Uses all available network interfaces to discover candidates.
    Relay,  ///< Uses only relay candidates such as STUN/TURN servers.
    NoHost, ///< Excludes local candidates.
    All,    ///< No candidates are used.
};

LIVEKIT_SIGNALING_API std::string toString(IceTransportPolicy policy);

} // namespace LiveKitCpp
