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
#include "livekit/signaling/TransportState.h"
#include "livekit/signaling/NetworkType.h"
#include "livekit/signaling/sfu/SignalTarget.h"
#include "livekit/signaling/sfu/Sdk.h"
#include "livekit/signaling/sfu/TrackSource.h"
#include "livekit/signaling/sfu/TrackType.h"
#include <cassert>

namespace LiveKitCpp
{

std::string toString(TransportState state)
{
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
        case NetworkType::NoNetwork:
            break;
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

std::string toString(TrackSource source)
{
    switch (source) {
        case TrackSource::Unknown:
            break;
        case TrackSource::Camera:
            return "camera";
        case TrackSource::Microphone:
            return "microphone";
        case TrackSource::ScreenShare:
            return "screen-share";
        case TrackSource::ScreenShareAudio:
            return "screen-share audio";
        default:
            assert(false);
            break;
    }
    return "unknown";
}

std::string toString(TrackType type)
{
    switch (type) {
        case TrackType::Audio:
            return "audio";
        case TrackType::Video:
            return "video";
        case TrackType::Data:
            break;
        default:
            assert(false);
            break;
    }
    return "data";
}

} // namespace LiveKitCpp
