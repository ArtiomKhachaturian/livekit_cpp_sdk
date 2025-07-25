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
#pragma once // DataExchangeListener.h
#include "livekit/signaling/sfu/ChatMessage.h"
#include "livekit/signaling/sfu/UserPacket.h"
#include "livekit/signaling/sfu/DataStreamHeader.h"
#include "livekit/signaling/sfu/DataStreamChunk.h"
#include "livekit/signaling/sfu/DataStreamTrailer.h"
#include <string>
#include <vector>

namespace LiveKitCpp
{

class DataChannel;

class DataExchangeListener
{
public:
    virtual void onError(std::string /*details*/ = {}) {}
    virtual void onUserPacket(UserPacket /*packet*/,
                              std::string /*participantIdentity*/,
                              std::vector<std::string> /*destinationIdentities*/ = {}) {}
    virtual void onChatMessage(ChatMessage /*message*/,
                               std::string /*participantIdentity*/,
                               std::vector<std::string> /*destinationIdentities*/ = {}) {}
    virtual void onDataStreamHeader(DataStreamHeader /*header*/,
                                    std::string /*participantIdentity*/,
                                    std::vector<std::string> /*destinationIdentities*/ = {}) {}
    virtual void onDataStreamChunk(DataStreamChunk /*chunk*/,
                                   std::string /*participantIdentity*/,
                                   std::vector<std::string> /*destinationIdentities*/ = {}) {}
    virtual void onDataStreamTrailer(DataStreamTrailer /*trailer*/,
                                     std::string /*participantIdentity*/,
                                     std::vector<std::string> /*destinationIdentities*/ = {}) {}
protected:
    virtual ~DataExchangeListener() = default;
};

} // namespace LiveKitCpp
