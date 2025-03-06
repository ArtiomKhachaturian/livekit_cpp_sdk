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
#pragma once // ConnectOptions.h
#include "LiveKitClientExport.h"
#include "rtc/ICEServer.h"
#include "rtc/ICETransportPolicy.h"
#include <chrono>
#include <vector>

namespace LiveKitCpp
{

using namespace std::chrono_literals;

// https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Types/Options/RoomOptions.swift#L20
/// Options used when establishing a connection.
struct ConnectOptions
{
    LIVEKIT_CLIENT_API ConnectOptions();
    /// Automatically subscribe to ``RemoteParticipant``'s tracks.
    /// Defaults to true.
    bool _autoSubscribe = true;
    
    /// The number of attempts to reconnect when the network disconnects.
    int _reconnectAttempts = 3;
    
    /// The delay between reconnect attempts.
    std::chrono::milliseconds _reconnectAttemptDelay = 2s;

    /// The timeout interval for the initial websocket connection.
    std::chrono::milliseconds _socketConnectTimeoutInterval = 10s;

    std::chrono::milliseconds _primaryTransportConnectTimeout = 10s;

    std::chrono::milliseconds _publisherTransportConnectTimeout = 10s;

    std::vector<ICEServer> _iceServers;

    IceTransportPolicy _iceTransportPolicy = IceTransportPolicy::All;

    /// LiveKit server protocol version to use. Generally, it's not recommended to change this.
    int _protocolVersion = {};
};

} // namespace LiveKitCpp
