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
#include "SignalParser.h"

namespace LiveKitCpp
{

std::optional<livekit::SignalResponse> SignalParser::parse(const void* data, size_t dataLen)
{
    return toProto<livekit::SignalResponse>(data, dataLen);
}

JoinResponse SignalParser::fromProto(const livekit::JoinResponse& in)
{
    JoinResponse out;
    if (in.has_room()) {
        out._room = fromProto(in.room());
    }
    if (in.has_participant()) {
        out._participant = fromProto(in.participant());
    }
    out._otherParticipants = fromProto<ParticipantInfo, livekit::ParticipantInfo>(in.other_participants());
    out._serverVersion = in.server_version();
    out._subscriberPrimary = in.subscriber_primary();
    out._alternativeUrl = in.alternative_url();
    if (in.has_client_configuration()) {
        out._clientConfiguration = fromProto(in.client_configuration());
    }
    out._serverRegion = in.server_region();
    out._pingTimeout = in.ping_timeout();
    out._pingInterval = in.ping_interval();
    if (in.has_server_info()) {
        out._serverInfo = fromProto(in.server_info());
    }
    out._sifTrailer = in.sif_trailer();
    out._enabledPublishCodecs = fromProto<Codec, livekit::Codec>(in.enabled_publish_codecs());
    out._fastPublish = in.fast_publish();
    return out;
}

TrickleRequest SignalParser::fromProto(const livekit::TrickleRequest& in)
{
    TrickleRequest out;
    out._candidateInit = in.candidateinit();
    out._target = fromProto(in.target());
    out._final = in.final();
    return out;
}

ParticipantUpdate SignalParser::fromProto(const livekit::ParticipantUpdate& in)
{
    ParticipantUpdate out;
    out._participants = fromProto<ParticipantInfo, livekit::ParticipantInfo>(in.participants());
    return out;
}

Room SignalParser::fromProto(const livekit::Room& in)
{
    Room out;
    out._sid = in.sid();
    out._name = in.name();
    out._emptyTimeout = in.empty_timeout();
    out._departureTimeout = in.departure_timeout();
    out._maxParticipants = in.max_participants();
    out._creationTime = in.creation_time();
    out._creationTimeMs = in.creation_time_ms();
    out._turnPassword = in.turn_password();
    out._enabledCodecs = fromProto<Codec, livekit::Codec>(in.enabled_codecs());
    out._metadata = in.metadata();
    out._numParticipants = in.num_participants();
    out._numPublishers = in.num_publishers();
    out._activeRecording = in.active_recording();
    if (in.has_version()) {
        out._version = fromProto(in.version());
    }
    return out;
}

Codec SignalParser::fromProto(const livekit::Codec& in)
{
    return {in.mime(), in.fmtp_line()};
}

TimedVersion SignalParser::fromProto(const livekit::TimedVersion& in)
{
    return {in.unix_micro(), in.ticks()};
}

ParticipantInfo SignalParser::fromProto(const livekit::ParticipantInfo& in)
{
    ParticipantInfo out;
    out._sid = in.sid();
    out._identity = in.identity();
    out._state = fromProto(in.state());
    out._tracks = fromProto<TrackInfo, livekit::TrackInfo>(in.tracks());
    out._metadata = in.metadata();
    out._joinedAt = in.joined_at();
    out.joinedAtMs = in.joined_at_ms();
    out._name = in.name();
    out._version = in.version();
    if (in.has_permission()) {
        out._permission = fromProto(in.permission());
    }
    out._region = in.region();
    out._isPublisher = in.is_publisher();
    out._kind = fromProto(in.kind());
    //out._attributes
    out._disconnectReason = fromProto(in.disconnect_reason());
    return out;
}

ParticipantKind SignalParser::fromProto(livekit::ParticipantInfo_Kind in)
{
    switch (in) {
        case livekit::ParticipantInfo_Kind_STANDARD:
            break;
        case livekit::ParticipantInfo_Kind_INGRESS:
            return ParticipantKind::Ingress;
        case livekit::ParticipantInfo_Kind_EGRESS:
            return ParticipantKind::Egress;
        case livekit::ParticipantInfo_Kind_SIP:
            return ParticipantKind::Sip;
        case livekit::ParticipantInfo_Kind_AGENT:
            return ParticipantKind::Agent;
        default: // TODO: log error
            assert(false);
            break;
    }
    return ParticipantKind::Standard;
}

ParticipantState SignalParser::fromProto(livekit::ParticipantInfo_State in)
{
    switch (in) {
        case livekit::ParticipantInfo_State_JOINING:
            return ParticipantState::Joining;
        case livekit::ParticipantInfo_State_JOINED:
            return ParticipantState::Joined;
        case livekit::ParticipantInfo_State_ACTIVE:
            return ParticipantState::Active;
        case livekit::ParticipantInfo_State_DISCONNECTED:
            break;
        default: // TODO: log error
            assert(false);
            break;
    }
    return ParticipantState::Disconnected;
}

ParticipantPermission SignalParser::fromProto(const livekit::ParticipantPermission& in)
{
    ParticipantPermission out;
    out._canSubscribe = in.can_subscribe();
    out._canPublish = in.can_publish();
    out._canPublish_data = in.can_publish_data();
    out._canPublishSources = fromProto<TrackSource, livekit::TrackSource>(in.can_publish_sources());
    out._hidden = in.hidden();
    out._recorder = in.recorder();
    out._canUpdateMetadata = in.can_update_metadata();
    out._agent = in.agent();
    out._canSubscribeMetrics = in.can_subscribe_metrics();
    return out;
}

DisconnectReason SignalParser::fromProto(livekit::DisconnectReason in)
{
    switch (in) {
        case livekit::UNKNOWN_REASON:
            break;
        case livekit::CLIENT_INITIATED:
            return DisconnectReason::ClientInitiated;
        case livekit::DUPLICATE_IDENTITY:
            return DisconnectReason::DuplicateIdentity;
        case livekit::SERVER_SHUTDOWN:
            return DisconnectReason::ServerShutdown;
        case livekit::PARTICIPANT_REMOVED:
            return DisconnectReason::ParticipantRemoved;
        case livekit::ROOM_DELETED:
            return DisconnectReason::RoomDeleted;
        case livekit::STATE_MISMATCH:
            return DisconnectReason::StateMismatch;
        case livekit::JOIN_FAILURE:
            return DisconnectReason::JoinFailure;
        case livekit::MIGRATION:
            return DisconnectReason::Migration;
        case livekit::SIGNAL_CLOSE:
            return DisconnectReason::SignalClose;
        case livekit::ROOM_CLOSED:
            return DisconnectReason::RoomClosed;
        case livekit::USER_UNAVAILABLE:
            return DisconnectReason::UserUnavailable;
        case livekit::USER_REJECTED:
            return DisconnectReason::UserRejected;
        case livekit::SIP_TRUNK_FAILURE:
            return DisconnectReason::SipTrunkFailure;
        default: // TODO: log error
            assert(false);
            break;
    }
    return DisconnectReason::UnknownReason;
}

TrackSource SignalParser::fromProto(livekit::TrackSource in)
{
    switch (in) {
        case livekit::UNKNOWN:
            break;
        case livekit::CAMERA:
            return TrackSource::Camera;
        case livekit::MICROPHONE:
            return TrackSource::Microphone;
        case livekit::SCREEN_SHARE:
            return TrackSource::ScreenShare;
        case livekit::SCREEN_SHARE_AUDIO:
            return TrackSource::ScreenShareAudio;
        default: // TODO: log error
            assert(false);
            break;
    }
    return TrackSource::Unknown;
}

TrackInfo SignalParser::fromProto(const livekit::TrackInfo& in)
{
    TrackInfo out;
    out._sid = in.sid();
    out._type = fromProto(in.type());
    out._name = in.name();
    out._muted = in.muted();
    out._width = in.width();
    out._height = in.height();
    out._simulcast = in.simulcast();
    out._disableDtx = in.disable_dtx();
    out._source = fromProto(in.source());
    out._layers = fromProto<VideoLayer, livekit::VideoLayer>(in.layers());
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    out._codecs = fromProto<SimulcastCodecInfo, livekit::SimulcastCodecInfo>(in.codecs());
    out._stereo = in.stereo();
    out._disableRed = in.disable_red();
    out._encryption = fromProto(in.encryption());
    out._stream = in.stream();
    if (in.has_version()) {
        out._version = fromProto(in.version());
    }
    out._audioFeatures = fromProto<AudioTrackFeature, livekit::AudioTrackFeature>(in.audio_features());
    out._backupCodecPolicy = fromProto(in.backup_codec_policy());
    return out;
}

VideoQuality SignalParser::fromProto(livekit::VideoQuality in)
{
    switch (in) {
        case livekit::LOW:
            break;
        case livekit::MEDIUM:
            return VideoQuality::Medium;
        case livekit::HIGH:
            return VideoQuality::High;
        case livekit::OFF:
            return VideoQuality::Off;
        default: // TODO: log error
            assert(false);
            break;
    }
    return VideoQuality::Low;
}

VideoLayer SignalParser::fromProto(const livekit::VideoLayer& in)
{
    VideoLayer out;
    out._quality = fromProto(in.quality());
    out._width = in.width();
    out._height = in.height();
    out._bitrate = in.bitrate();
    out._ssrc = in.ssrc();
    return out;
}

TrackType SignalParser::fromProto(livekit::TrackType in)
{
    switch (in) {
        case livekit::AUDIO:
            break;
        case livekit::VIDEO:
            return TrackType::Video;
        case livekit::DATA:
            return TrackType::Data;
        default: // TODO: log error
            assert(false);
            break;
    }
    return TrackType::Audio;
}

SimulcastCodecInfo SignalParser::fromProto(const livekit::SimulcastCodecInfo& in)
{
    SimulcastCodecInfo out;
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    out._cid = in.cid();
    out._layers = fromProto<VideoLayer, livekit::VideoLayer>(in.layers());
    return out;
}

BackupCodecPolicy SignalParser::fromProto(livekit::BackupCodecPolicy in)
{
    switch (in) {
        case livekit::REGRESSION:
            break;
        case livekit::SIMULCAST:
            return BackupCodecPolicy::Simulcast;
        default: // TODO: log error
            assert(false);
            break;
    }
    return BackupCodecPolicy::Regression;
}

EncryptionType SignalParser::fromProto(livekit::Encryption_Type in)
{
    switch (in) {
        case livekit::Encryption_Type_NONE:
            break;
        case livekit::Encryption_Type_GCM:
            return EncryptionType::Gcm;
        case livekit::Encryption_Type_CUSTOM:
            return EncryptionType::Custom;
        default: // TODO: log error
            assert(false);
            break;
    }
    return EncryptionType::None;
}

AudioTrackFeature SignalParser::fromProto(livekit::AudioTrackFeature in)
{
    switch (in) {
        case livekit::TF_STEREO:
            break;
        case livekit::TF_NO_DTX:
            return AudioTrackFeature::TFNoDtx;
        case livekit::TF_AUTO_GAIN_CONTROL:
            return AudioTrackFeature::TFAutoGainControl;
        case livekit::TF_ECHO_CANCELLATION:
            return AudioTrackFeature::TFEchocancellation;
        case livekit::TF_NOISE_SUPPRESSION:
            return AudioTrackFeature::TFNoiseSuppression;
        case livekit::TF_ENHANCED_NOISE_CANCELLATION:
            return AudioTrackFeature::TFEnhancedNoiseCancellation;
        default: // TODO: log error
            assert(false);
            break;
    }
    return AudioTrackFeature::TFStereo;
}

ClientConfigSetting SignalParser::fromProto(livekit::ClientConfigSetting in)
{
    switch (in) {
        case livekit::UNSET:
            break;
        case livekit::DISABLED:
            return ClientConfigSetting::Disabled;
        case livekit::ENABLED:
            return ClientConfigSetting::Enabled;
        default: // TODO: log error
            assert(false);
            break;
    }
    return ClientConfigSetting::Unset;
}

ClientConfiguration SignalParser::fromProto(const livekit::ClientConfiguration& in)
{
    ClientConfiguration out;
    out._video = fromProto(in.video());
    out._screen = fromProto(in.screen());
    out._resumeConnection = fromProto(in.resume_connection());
    out._disabledCodecs = fromProto(in.disabled_codecs());
    out._forceRelay = fromProto(in.force_relay());
    return out;
}

DisabledCodecs SignalParser::fromProto(const livekit::DisabledCodecs& in)
{
    DisabledCodecs out;
    out._codecs = fromProto<Codec, livekit::Codec>(in.codecs());
    out._publish = fromProto<Codec, livekit::Codec>(in.publish());
    return out;
}

VideoConfiguration SignalParser::fromProto(const livekit::VideoConfiguration& in)
{
    VideoConfiguration out;
    out._hardwareEncoder = fromProto(in.hardware_encoder());
    return out;
}

ServerEdition SignalParser::fromProto(livekit::ServerInfo_Edition in)
{
    switch (in) {
        case livekit::ServerInfo_Edition_Standard:
            break;
        case livekit::ServerInfo_Edition_Cloud:
            return ServerEdition::Cloud;
        default: // TODO: log error
            assert(false);
            break;
    }
    return ServerEdition::Standard;
}

ServerInfo SignalParser::fromProto(const livekit::ServerInfo& in)
{
    ServerInfo out;
    out._edition = fromProto(in.edition());
    out._version = in.version();
    out._protocol = in.protocol();
    out._region = in.region();
    out._nodeId = in.node_id();
    out._debugInfo = in.debug_info();
    out._agentProtocol = in.agent_protocol();
    return out;
}

SignalTarget SignalParser::fromProto(livekit::SignalTarget in)
{
    switch (in) {
        case livekit::PUBLISHER:
            break;
        case livekit::SUBSCRIBER:
            return SignalTarget::Subscriber;
        default: // TODO: log error
            assert(false);
            break;
    }
    return SignalTarget::Publisher;
}

template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
std::vector<TCppType> SignalParser::fromProto(const TProtoBufRepeated& in)
{
    if (const auto size = in.size()) {
        std::vector<TCppType> out;
        out.reserve(size_t(size));
        for (const auto& val : in) {
            out.push_back(fromProto(TProtoBufType(val)));
        }
        return out;
    }
    return {};
}

} // namespace LiveKitCpp
