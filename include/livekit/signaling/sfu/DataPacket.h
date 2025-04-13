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
#pragma once // DataPacket.h
#include "livekit/signaling/sfu/ChatMessage.h"
#include "livekit/signaling/sfu/DataPacketKind.h"
#include "livekit/signaling/sfu/UserPacket.h"
#include "livekit/signaling/sfu/DataStreamHeader.h"
#include "livekit/signaling/sfu/DataStreamChunk.h"
#include "livekit/signaling/sfu/DataStreamTrailer.h"
#include <variant>

namespace LiveKitCpp
{

struct DataPacket
{
    [[deprecated]] DataPacketKind _kind = {};
    // participant identity of user that sent the message
    std::string _participantIdentity;
    // identities of participants who will receive the message (sent to all by default)
    std::vector<std::string> _destinationIdentities;
    std::variant<std::nullptr_t,
                 ChatMessage,
                 UserPacket,
                 DataStreamHeader,
                 DataStreamChunk,
                 DataStreamTrailer> _value;
};
	
} // namespace LiveKitCpp
