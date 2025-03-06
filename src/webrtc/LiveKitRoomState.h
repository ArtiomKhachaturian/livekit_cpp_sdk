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
#pragma once // LiveKitRoomState.h
#include "ReconnectMode.h"
#include "ConnectOptions.h"
#include "RoomOptions.h"
#include "Transport.h"
#include "State.h"
#include "rtc/ClientConfiguration.h"
#include "rtc/ICEServer.h"
#include "rtc/ServerInfo.h"
#include <api/peer_connection_interface.h>
#include <chrono>
#include <string>

namespace LiveKitCpp
{

struct JoinResponse;
struct ReconnectResponse;

// https://github.com/livekit/client-sdk-swift/blob/b7c2ebc7cde9af7018d475961d7e7957f69d310f/Sources/LiveKit/Core/Room.swift#L100-L205
struct LiveKitRoomState
{
    LiveKitRoomState(const ConnectOptions& connectOptions,
                     const RoomOptions& roomOptions);
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const std::vector<ICEServer>& iceServers = {},
                          const std::optional<ClientConfiguration>& cc = {}) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const JoinResponse& response) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const ReconnectResponse& response) const;
    ConnectOptions _connectOptions;
    RoomOptions _roomOptions;
    std::string _sid;
    std::string _name;
    std::string _metadata;
    // var remoteParticipants = [Participant.Identity: RemoteParticipant]()
    // var activeSpeakers = [Participant]()
    std::chrono::time_point<std::chrono::system_clock> _creationTime;
    bool _isRecording = false;
    int _maxParticipants = 0;
    int _numParticipants = 0;
    int _numPublishers = 0;
    std::optional<ServerInfo> _serverInfo;
    
    // Engine
    std::string _url;
    std::string _token;
    // preferred reconnect mode which will be used only for next attempt
    std::optional<ReconnectMode> _nextReconnectMode;
    std::optional<ReconnectMode> isReconnectingWithMode;
    State _connectionState = State::Disconnected;
    bool _hasPublished = false;
    std::unique_ptr<Transport> _publisher;
    std::unique_ptr<Transport> _subscriber;
    bool _isSubscriberPrimary = false;
};

} // namespace LiveKitCpp
