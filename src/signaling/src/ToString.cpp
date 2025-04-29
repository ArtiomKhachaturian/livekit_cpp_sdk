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
#include "livekit/signaling/sfu/VideoQuality.h"
#include "livekit/signaling/sfu/SubscriptionError.h"
#include "livekit/signaling/sfu/StreamState.h"
#include "livekit/signaling/sfu/ServerEdition.h"
#include "livekit/signaling/sfu/RequestResponseReason.h"
#include "livekit/signaling/sfu/ParticipantKind.h"
#include "livekit/signaling/sfu/LeaveRequestAction.h"
#include "livekit/signaling/sfu/ICETransportPolicy.h"
#include "livekit/signaling/sfu/EncryptionType.h"
#include "livekit/signaling/sfu/DisconnectReason.h"
#include "livekit/signaling/sfu/DataStreamTextHeaderOperationType.h"
#include "livekit/signaling/sfu/DataPacketKind.h"
#include "livekit/signaling/sfu/ConnectionQuality.h"
#include "livekit/signaling/sfu/ClientConfigSetting.h"
#include "livekit/signaling/sfu/CandidateProtocol.h"
#include "livekit/signaling/sfu/BackupCodecPolicy.h"
#include "livekit/signaling/sfu/AudioTrackFeature.h"


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
        case NetworkType::Unknown:
            break;
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

std::string toString(VideoQuality quality)
{
    switch (quality) {
        case VideoQuality::Low:
            return "low";
        case VideoQuality::Medium:
            return "medium";
        case VideoQuality::High:
            return "high";
        case VideoQuality::Off:
            break;
        default:
            assert(false);
            break;
    }
    return "off";
}

std::string toString(SubscriptionError error)
{
    switch (error) {
        case SubscriptionError::Unknown:
            break;
        case SubscriptionError::CodecUnsupported:
            return "codec unsupported";
        case SubscriptionError::TrackNotfound:
            return "track not found";
        default:
            assert(false);
            break;
    }
    return "unknown";
}

std::string toString(StreamState state)
{
    switch (state) {
        case StreamState::Active:
            return "active";
        case StreamState::Paused:
            break;
        default:
            assert(false);
            break;
    }
    return "paused";
}

std::string toString(ServerEdition edition)
{
    switch (edition) {
        case ServerEdition::Standard:
            break;
        case ServerEdition::Cloud:
            return "cloud";
        default:
            assert(false);
            break;
    }
    return "standard";
}

std::string toString(RequestResponseReason reason)
{
    switch (reason) {
        case RequestResponseReason::Ok:
            return "ok";
        case RequestResponseReason::NotFound:
            break;
        case RequestResponseReason::NotAllowed:
            return "not allowed";
        case RequestResponseReason::LimitExceeded:
            return "limit exceeded";
        default:
            assert(false);
            break;
    }
    return "not found";
}

std::string toString(ParticipantKind kind)
{
    switch (kind) {
        case ParticipantKind::Standard:
            break;
        case ParticipantKind::Ingress:
            return "ingress";
        case ParticipantKind::Egress:
            return "egress";
        case ParticipantKind::Sip:
            return "sip";
        case ParticipantKind::Agent:
            return "agent";
        default:
            assert(false);
            break;
    }
    return "standard";
}

std::string toString(LeaveRequestAction action)
{
    switch (action) {
        case LeaveRequestAction::Disconnect:
            break;
        case LeaveRequestAction::Resume:
            return "resume";
        case LeaveRequestAction::Reconnect:
            return "reconnect";
        default:
            assert(false);
            break;
    }
    return "disconnect";
}

std::string toString(IceTransportPolicy policy)
{
    switch (policy) {
        case IceTransportPolicy::None:
            break;
        case IceTransportPolicy::Relay:
            return "relay";
        case IceTransportPolicy::NoHost:
            return "no host";
        case IceTransportPolicy::All:
            return "all";
        default:
            assert(false);
            break;
    }
    return "none";
}

std::string toString(EncryptionType type)
{
    switch (type) {
        case EncryptionType::None:
            return "";
        case EncryptionType::Gcm:
            return "gcm";
        case EncryptionType::Custom:
            return "custom";
        default:
            assert(false);
            break;
    }
    return "none";
}

std::string toString(DisconnectReason reason)
{
    switch (reason) {
        case DisconnectReason::UnknownReason:
            break;
        case DisconnectReason::ClientInitiated:
            return "client initiated";
        case DisconnectReason::DuplicateIdentity:
            return "duplicate identity";
        case DisconnectReason::ServerShutdown:
            return "server shutdown";
        case DisconnectReason::ParticipantRemoved:
            return "participant removed";
        case DisconnectReason::RoomDeleted:
            return "room deleted";
        case DisconnectReason::StateMismatch:
            return "state mismatch";
        case DisconnectReason::JoinFailure:
            return "join failure";
        case DisconnectReason::Migration:
            return "migration";
        case DisconnectReason::SignalClose:
            return "signal close";
        case DisconnectReason::RoomClosed:
            return "room closed";
        case DisconnectReason::UserUnavailable:
            return "user unavailable";
        case DisconnectReason::UserRejected:
            return "user rejected";
        case DisconnectReason::SipTrunkFailure:
            return "sip trunk failure";
        case DisconnectReason::ConnectionTimeout:
            return "connection timeout";
        default:
            assert(false);
            break;
    }
    return "unknown";
}

std::string toString(DataStreamTextHeaderOperationType type)
{
    switch (type) {
        case DataStreamTextHeaderOperationType::Create:
            break;
        case DataStreamTextHeaderOperationType::Update:
            return "update";
        case DataStreamTextHeaderOperationType::Delete:
            return "delete";
        case DataStreamTextHeaderOperationType::Reaction:
            return "reaction";
        default:
            assert(false);
            break;
    }
    return "create";
}

std::string toString(DataPacketKind kind)
{
    switch (kind) {
        case DataPacketKind::Reliable:
            break;
        case DataPacketKind::Lossy:
            return "lossy";
        default:
            assert(false);
            break;
    }
    return "reliable";
}

std::string toString(ConnectionQuality quality)
{
    switch (quality) {
        case ConnectionQuality::Poor:
            break;
        case ConnectionQuality::Good:
            return "good";
        case ConnectionQuality::Excellent:
            return "excellent";
        case ConnectionQuality::Lost:
            return "lost";
        default:
            assert(false);
            break;
    }
    return "poor";
}

std::string toString(ClientConfigSetting setting)
{
    switch (setting) {
        case ClientConfigSetting::Unset:
            break;
        case ClientConfigSetting::Disabled:
            return "disabled";
        case ClientConfigSetting::Enabled:
            return "enabled";
        default:
            assert(false);
            break;
    }
    return "unset";
}

std::string toString(CandidateProtocol protocol)
{
    switch (protocol) {
        case CandidateProtocol::Udp:
            break;
        case CandidateProtocol::Tcp:
            return "tcp";
        case CandidateProtocol::Tls:
            return "tls";
        default:
            assert(false);
            break;
    }
    return "udp";
}

std::string toString(BackupCodecPolicy policy)
{
    switch (policy) {
        case BackupCodecPolicy::PrefererRegression:
            break;
        case BackupCodecPolicy::Regression:
            return "regression";
        case BackupCodecPolicy::Simulcast:
            return "simulcast";
        default:
            assert(false);
            break;
    }
    return "preferer regression";
}

std::string toString(AudioTrackFeature feature)
{
    switch (feature) {
        case AudioTrackFeature::Stereo:
            break;
        case AudioTrackFeature::NoDtx:
            return "no dtx";
        case AudioTrackFeature::AutoGainControl:
            return "auto gain control";
        case AudioTrackFeature::Echocancellation:
            return "echo cancellation";
        case AudioTrackFeature::NoiseSuppression:
            return "noise suppression";
        case AudioTrackFeature::EnhancedNoiseCancellation:
            return "enhanced noise cancellation";
        default:
            assert(false);
            break;
    }
    return "stereo";
}


} // namespace LiveKitCpp
