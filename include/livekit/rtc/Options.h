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
#include "livekit/signaling/sfu/ClientInfo.h"
#include "livekit/signaling/sfu/ICEServer.h"
#include "livekit/signaling/sfu/ICETransportPolicy.h"
#include <chrono>
#include <optional>
#include <memory>
#include <string>
#include <vector>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

using namespace std::chrono_literals;

/// Options used when establishing a connection.
struct Options
{
    /// Automatically subscribe to ``RemoteParticipant``'s tracks.
    /// Defaults to true.
    bool _autoSubscribe = true;
    
    bool _adaptiveStream = true; // maybe std::optional<>?
    
    std::string _publish;
    
    std::optional<ClientInfo> _clientsInfo;
    
    std::string _prefferedAudioEncoder;
    
    std::string _prefferedVideoEncoder;
    
    // Whether to use the NetEq "fast mode" which will accelerate audio quicker
    // if it falls behind.
    std::optional<bool> _audioJitterBufferFastAccelerate;
    
    // The minimum delay in milliseconds for the audio jitter buffer.
    std::optional<int32_t> _audioJitterBufferMinDelayMs;
    
    // The maximum number of packets that can be stored in the NetEq audio
    // jitter buffer. Can be reduced to lower tolerated audio latency.
    std::optional<int32_t> _audioJitterBufferMaxPackets;
    
    /**
     * @brief Delay applied to the renegotiation process to
     * minimize frequent exchange of SDP between client and server.
     *
     * This variable defines the minimum delay between successive renegotiation attempts in cases where
     * the user adds or removes multiple media tracks within a short period of time. It ensures that
     * the renegotiation process does not trigger excessively, allowing for more efficient handling of
     * rapid changes to the media session configuration.
     *
     * @note The default value is set to 50 milliseconds, 0 (zero) means that no delay, upper limit is 100 ms
     */
    std::chrono::milliseconds _negotiationDelay = 50ms;
    
    /// The number of attempts to reconnect when the network disconnects.
    int _reconnectAttempts = 3;
    
    /// The delay between reconnect attempts.
    std::chrono::milliseconds _reconnectAttemptDelay = 2s;

    /// The timeout interval for the initial websocket connection.
    /*std::chrono::milliseconds _socketConnectTimeoutInterval = 10s;

    std::chrono::milliseconds _primaryTransportConnectTimeout = 10s;

    std::chrono::milliseconds _publisherTransportConnectTimeout = 10s;*/

    std::vector<ICEServer> _iceServers;

    // generally, it's not recommended to change this
    IceTransportPolicy _iceTransportPolicy = IceTransportPolicy::All;
};

} // namespace LiveKitCpp
