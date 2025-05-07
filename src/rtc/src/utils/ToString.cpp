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
//#include " LiveKitError.h" // ToString.cpp
#include "livekit/rtc/LiveKitError.h"
#include "livekit/rtc/media/VideoScalabilityMode.h"
#include <cassert>

namespace LiveKitCpp
{

std::string toString(LiveKitError error)
{
    switch (error) {
        case LiveKitError::RTC:
            return "RTC failure";
        case LiveKitError::Transport:
            return "the signal websocket was failed";
        case LiveKitError::ServerPingTimedOut:
            return "the server didn't respond to the ping signal (time out)";
        case LiveKitError::ServerDuplicateIdentity:
            return "another participant with the same identity has joined the room";
        case LiveKitError::ServerShutdown:
            return "the server instance is shutting down";
        case LiveKitError::ServerParticipantRemoved:
            return "Roomservice.Removeparticipant was called";
        case LiveKitError::ServerRoomDeleted:
            return "Roomservice.Deleteroom was called";
        case LiveKitError::ServerStateMismatch:
            return "the client is attempting to resume a session, but server is not aware of it";
        case LiveKitError::ServerJoinFailure:
            return "client was unable to connect fully";
        case LiveKitError::ServerMigration:
            return "cloud-only, the server requested participant to migrate the connection elsewhere";
        case LiveKitError::ServerSignalClose:
            return "the signal websocket was closed unexpectedly";
        case LiveKitError::ServerRoomClosed:
            return "the room was closed, due to all standard and ingress participants having left";
        case LiveKitError::ServerUserUnavailable:
            return "sip callee did not respond in time";
        case LiveKitError::ServerUserRejected:
            return "sip callee rejected the call (busy)";
        case LiveKitError::ServerSipTrunkFailure:
            return "sip protocol failure or unexpected response";
        default:
            assert(false);
            break;
    }
    return {};
}

std::string toString(VideoScalabilityMode type)
{
    switch (type) {
        case VideoScalabilityMode::Auto:
            break;
        case VideoScalabilityMode::L1T1:
            return "L1T1";
        case VideoScalabilityMode::L1T2:
            return "L1T2";
        case VideoScalabilityMode::L1T3:
            return "L1T3";
        case VideoScalabilityMode::L2T1:
            return "L2T1";
        case VideoScalabilityMode::L2T2:
            return "L2T2";
        case VideoScalabilityMode::L2T3:
            return "L2T3";
        case VideoScalabilityMode::L3T1:
            return "L3T1";
        case VideoScalabilityMode::L3T2:
            return "L3T2";
        case VideoScalabilityMode::L3T3:
            return "L3T3";
        case VideoScalabilityMode::L2T1h:
            return "L2T1h";
        case VideoScalabilityMode::L2T2h:
            return "L2T2h";
        case VideoScalabilityMode::L2T3h:
            return "L2T3h";
        case VideoScalabilityMode::L3T1h:
            return "L3T1h";
        case VideoScalabilityMode::L3T2h:
            return "L3T2h";
        case VideoScalabilityMode::L3T3h:
            return "L3T3h";
        case VideoScalabilityMode::S2T1:
            return "S2T1";
        case VideoScalabilityMode::S2T2:
            return "S2T2";
        case VideoScalabilityMode::S2T3:
            return "S2T3";
        case VideoScalabilityMode::S2T1h:
            return "S2T1h";
        case VideoScalabilityMode::S2T2h:
            return "S2T2h";
        case VideoScalabilityMode::S2T3h:
            return "S2T3h";
        case VideoScalabilityMode::S3T1:
            return "S3T1";
        case VideoScalabilityMode::S3T2:
            return "S3T2";
        case VideoScalabilityMode::S3T3:
            return "S3T3";
        case VideoScalabilityMode::S3T1h:
            return "S3T1h";
        case VideoScalabilityMode::S3T2h:
            return "S3T2h";
        case VideoScalabilityMode::S3T3h:
            return "S3T3h";
        case VideoScalabilityMode::L2T2Key:
            return "L2T2_KEY";
        case VideoScalabilityMode::L2T2KeyShift:
            return "L2T2_KEY_SHIFT";
        case VideoScalabilityMode::L2T3Key:
            return "L2T3_KEY";
        case VideoScalabilityMode::L2T3KeyShift:
            return "L2T3_KEY_SHIFT";
        case VideoScalabilityMode::L3T1Key:
            return "L3T1_KEY";
        case VideoScalabilityMode::L3T2Key:
            return "L3T2_KEY";
        case VideoScalabilityMode::L3T2KeyShift:
            return "L3T2_KEY_SHIFT";
        case VideoScalabilityMode::L3T3Key:
            return "L3T3_KEY";;
        case VideoScalabilityMode::L3T3KeyShift:
            return "L3T3_KEY_SHIFT";
        default:
            assert(false);
            break;
    }
    return {};
}

} // namespace LiveKitCpp
