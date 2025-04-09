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
#pragma once // LiveKitError.h
#include "LiveKitClientExport.h"
#include <string>

namespace LiveKitCpp
{

enum class LiveKitError
{
    RTC,
    Transport,
    // Server errors
    ServerPingTimedOut,
    // Another Participant With The Same Identity Has Joined The Room
    ServerDuplicateIdentity,
    // The Server Instance Is Shutting Down
    ServerShutdown,
    // Roomservice.Removeparticipant Was Called
    ServerParticipantRemoved,
    // Roomservice.Deleteroom Was Called
    ServerRoomDeleted,
    // The Client Is Attempting To Resume A Session, But Server Is Not Aware Of It
    ServerStateMismatch,
    // Client Was Unable To Connect Fully
    ServerJoinFailure,
    // Cloud-Only, The Server Requested Participant To Migrate The Connection Elsewhere
    ServerMigration,
    // The Signal Websocket Was Closed Unexpectedly
    ServerSignalClose,
    // The Room Was Closed, Due To All Standard And Ingress Participants Having Left
    ServerRoomClosed,
    // Sip Callee Did Not Respond In Time
    ServerUserUnavailable,
    // Sip Callee Rejected The Call (Busy)
    ServerUserRejected,
    // Sip Protocol Failure Or Unexpected Response
    ServerSipTrunkFailure
};

LIVEKIT_CLIENT_API std::string toString(LiveKitError error);

} // namespace LiveKitCpp
