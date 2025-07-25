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
#pragma once // Room.h
#include "livekit/signaling/sfu/Codec.h"
#include "livekit/signaling/sfu/TimedVersion.h"
#include <optional>
#include <string>
#include <vector>

namespace LiveKitCpp
{

struct RoomInfo
{
    std::string _sid;
    std::string _name;
    uint32_t _emptyTimeout = {};
    uint32_t _departureTimeout = {};
    uint32_t _maxParticipants = {};
    int64_t _creationTime = {};
    int64_t _creationTimeMs = {};
    std::string _turnPassword;
    std::vector<Codec> _enabledCodecs;
    std::string _metadata;
    uint32_t _numParticipants = {};
    uint32_t _numPublishers = {};
    bool _activeRecording = {};
    std::optional<TimedVersion> _version;
};

} // namespace LiveKitCpp
