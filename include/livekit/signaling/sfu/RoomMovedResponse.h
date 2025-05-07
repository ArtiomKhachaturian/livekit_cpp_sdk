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
#pragma once // RoomMovedResponse.h
#include "livekit/signaling/sfu/RoomInfo.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"
#include <string>
#include <vector>

namespace LiveKitCpp
{

// notify to the participant when they have been moved to a new room
struct RoomMovedResponse
{
    // information about the new room
    RoomInfo _room;
    // new reconnect token that can be used to reconnect to the new room
    std::string _token;
    ParticipantInfo _participant;
    std::vector<ParticipantInfo> _otherParticipants;
};

} // namespace LiveKitCpp
