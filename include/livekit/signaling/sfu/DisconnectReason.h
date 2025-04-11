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
#pragma once // DisconnectReason.h

namespace LiveKitCpp
{

enum class DisconnectReason
{
    UnknownReason = 0,
    // The Client Initiated The Disconnect
    ClientInitiated = 1,
    // Another Participant With The Same Identity Has Joined The Room
    DuplicateIdentity = 2,
    // The Server Instance Is Shutting Down
    ServerShutdown = 3,
    // Roomservice.Removeparticipant Was Called
    ParticipantRemoved = 4,
    // Roomservice.Deleteroom Was Called
    RoomDeleted = 5,
    // The Client Is Attempting To Resume A Session, But Server Is Not Aware Of It
    StateMismatch = 6,
    // Client Was Unable To Connect Fully
    JoinFailure = 7,
    // Cloud-Only, The Server Requested Participant To Migrate The Connection Elsewhere
    Migration = 8,
    // The Signal Websocket Was Closed Unexpectedly
    SignalClose = 9,
    // The Room Was Closed, Due To All Standard And Ingress Participants Having Left
    RoomClosed = 10,
    // Sip Callee Did Not Respond In Time
    UserUnavailable = 11,
    // Sip Callee Rejected The Call (Busy)
    UserRejected = 12,
    // Sip Protocol Failure Or Unexpected Response
    SipTrunkFailure = 13,
};

} // namespace LiveKitCpp
