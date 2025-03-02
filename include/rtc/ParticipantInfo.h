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
#pragma once // ParticipantInfo.h
#include "rtc/ParticipantPermission.h"
#include "rtc/ParticipantKind.h"
#include "rtc/ParticipantState.h"
#include "rtc/TrackInfo.h"
#include "rtc/DisconnectReason.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace LiveKitCpp
{

struct ParticipantInfo
{
    std::string _sid;
    std::string _identity;
    ParticipantState _state = {};
    std::vector<TrackInfo> _tracks;
    std::string _metadata;
    // timestamp when participant joined room, in seconds
    int64_t _joinedAt = {};
    // timestamp when participant joined room, in milliseconds
    int64_t joinedAtMs = {};
    std::string _name;
    uint32_t _version = {};
    ParticipantPermission _permission;
    std::string _region;
    // indicates the participant has an active publisher connection
    // and can publish to the server
    bool _isPublisher = {};
    ParticipantKind _kind = {};
    std::unordered_map<std::string, std::string> _attributes;
    DisconnectReason _disconnectReason = {};
};

} // namespace LiveKitCpp
