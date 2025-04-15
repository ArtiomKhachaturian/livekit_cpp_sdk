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
#pragma once // SessionListener.h
#include "livekit/rtc/LiveKitError.h"
#include "livekit/rtc/ParticipantListener.h"
#include "livekit/rtc/SessionState.h"
#include "livekit/signaling/sfu/ChatMessage.h"
#include "livekit/signaling/sfu/UserPacket.h"
#include <string>

namespace LiveKitCpp
{

class SessionListener : public ParticipantListener
{
public:
    virtual void onStateChanged(SessionState /*state*/) {}
    virtual void onError(LiveKitError /*error*/, const std::string& /*what*/ = {}) {}
    virtual void onLocalParticipantJoined() {}
    virtual void onLocalParticipantLeaved() {}
    virtual void onRemoteParticipantAdded(const std::string& /*remoteParticipantSid*/) {}
    virtual void onRemoteParticipantRemoved(const std::string& /*remoteParticipantSid*/) {}
    // chat
    virtual void onUserPacketReceived(const UserPacket& /*packet*/,
                                      const std::string& /*participantIdentity*/,
                                      const std::vector<std::string>& /*destinationIdentities*/ = {}) {}
    virtual void onChatMessageReceived(const ChatMessage& /*message*/,
                                       const std::string& /*participantIdentity*/,
                                       const std::vector<std::string>& /*destinationIdentities*/ = {}) {}
protected:
    virtual ~SessionListener() = default;
};

} // namespace LiveKitCpp
