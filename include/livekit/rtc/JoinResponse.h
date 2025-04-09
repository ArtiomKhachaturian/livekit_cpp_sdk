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
#pragma once // JoinResponse.h
#include "rtc/ClientConfiguration.h"
#include "rtc/Codec.h"
#include "rtc/ICEServer.h"
#include "rtc/RoomInfo.h"
#include "rtc/ParticipantInfo.h"
#include "rtc/ServerInfo.h"
#include <string>
#include <vector>

namespace LiveKitCpp
{

// sent when join is accepted
struct JoinResponse
{
    RoomInfo _room;
    ParticipantInfo _participant;
    std::vector<ParticipantInfo> _otherParticipants;
     // deprecated. use server_info.version instead.
    [[deprecated("Use _serverInfo._version instead.")]] std::string _serverVersion;
    std::vector<ICEServer> _iceServers;
     // use subscriber as the primary PeerConnection
    bool _subscriberPrimary = {};
     // when the current server isn't available, return alternate url to retry connection
     // when this is set, the other fields will be largely empty
    std::string _alternativeUrl;
    ClientConfiguration _clientConfiguration;
     // deprecated. use server_info.region instead.
    [[deprecated("Use _serverInfo._region instead.")]] std::string _serverRegion;
    int32_t _pingTimeout = {};
    int32_t _pingInterval = {};
    std::optional<ServerInfo> _serverInfo;
    // Server-Injected-Frame byte trailer, used to identify unencrypted frames when e2ee is enabled
    std::string _sifTrailer;
    std::vector<Codec> _enabledPublishCodecs;
    // when set, client should attempt to establish publish peer connection when joining room to speed up publishing
    bool _fastPublish = {};
};

} // namespace LiveKitCpp
