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
#pragma once // LiveKitRoomOptions.h
#include <chrono>

namespace LiveKitCpp
{

using namespace std::chrono_literals;

// https://github.com/livekit/client-sdk-swift/blob/b7c2ebc7cde9af7018d475961d7e7957f69d310f/Sources/LiveKit/Types/Options/RoomOptions.swift
// https://github.com/livekit/client-sdk-swift/blob/b7c2ebc7cde9af7018d475961d7e7957f69d310f/Sources/LiveKit/Types/Options/ConnectOptions.swift
struct LiveKitRoomOptions
{
    /// AdaptiveStream lets LiveKit automatically manage quality of subscribed
    /// video tracks to optimize for bandwidth and CPU.
    /// When attached video elements are visible, it'll choose an appropriate
    /// resolution based on the size of largest video element it's attached to.
    ///
    /// When none of the video elements are visible, it'll temporarily pause
    /// the data flow until they are visible again.
    ///
    bool _adaptiveStream = false;
    
    /// Dynamically pauses video layers that are not being consumed by any subscribers,
    /// significantly reducing publishing CPU and bandwidth usage.
    ///
    bool _dynacast = false;

    
    bool _stopLocalTrackOnUnpublish = true;

    bool _suspendLocalVideoTracksInBackground = true;

    bool _reportRemoteTrackStatistics = false;
    
    /// Automatically subscribe to ``RemoteParticipant``'s tracks.
    /// Defaults to true.
    bool _autoSubscribe = true;

    /// The number of attempts to reconnect when the network disconnects.
    unsigned _reconnectAttempts = 3U;

    /// The delay between reconnect attempts.
    std::chrono::milliseconds _reconnectAttemptDelay = 2s;

    /// The timeout interval for the initial websocket connection.
    std::chrono::milliseconds _websocketConnectTimeoutInterval = 10s;

    std::chrono::milliseconds _primaryTransportConnectTimeout = 10s;

    std::chrono::milliseconds _publisherTransportConnectTimeout = 10s;
};

} // namespace LiveKitCpp
