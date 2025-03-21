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
#include "LiveKitError.h" // ToString.cpp
#include "TransportState.h"
#include "NetworkType.h"
#include "rtc/SignalTarget.h"
#include "rtc/Sdk.h"
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

std::string toString(TransportState state) {
    switch (state) {
        case TransportState::Connecting:
            return "connecting";
        case TransportState::Connected:
            return "connected";
        case TransportState::Disconnecting:
            return "disconnecting";
        case TransportState::Disconnected:
            return "disconnected";
        default:
            assert(false);
            break;
    }
    return "unknown";
}

std::string toString(NetworkType state)
{
    switch (state) {
        case NetworkType::WiFi:
            return "wifi";
        case NetworkType::Wired:
            return "wired";
        case NetworkType::Cellular:
            return "cellular";
        case NetworkType::Vpn:
            return "vpn";
        default:
            assert(false);
            break;
    }
    return {};
}

std::string toString(SignalTarget target)
{
    switch (target) {
        case SignalTarget::Publisher:
            return "publisher";
        case SignalTarget::Subscriber:
            return "subscriber";
        default:
            assert(false);
            break;
    }
    return {};
}

std::string toString(SDK sdk)
{
    switch (sdk) {
        case SDK::Unknown:
            break;
        case SDK::JS:
            return "JS";
        case SDK::Swift:
            return "SWIFT";
        case SDK::Android:
            return "ANDROID";
        case SDK::Flutter:
            return "FLUTTER";
        case SDK::GO:
            return "GO";
        case SDK::Unity:
            return "UNITY";
        case SDK::ReactNative:
            return "REACT_NATIVE";
        case SDK::Rust:
            return "RUST";
        case SDK::Python:
            return "PYTHON";
        case SDK::CPP:
            return "CPP";
        case SDK::UnityWeb:
            return "UNITY_WEB";
        case SDK::Node:
            return "NODE";
        default:
            assert(false);
            break;
    }
    return "UNKNOWN";
}

} // namespace LiveKitCpp
