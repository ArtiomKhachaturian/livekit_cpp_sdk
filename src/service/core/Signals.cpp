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
#include "Signals.h"

namespace LiveKitCpp
{

JoinResponse Signals::map(const livekit::JoinResponse& in)
{
    JoinResponse out;
    if (in.has_room()) {
        out._room = map(in.room());
    }
    if (in.has_participant()) {
        out._participant = map(in.participant());
    }
    out._otherParticipants = map<ParticipantInfo, livekit::ParticipantInfo>(in.other_participants());
    out._serverVersion = in.server_version();
    out._iceServers = map<ICEServer, livekit::ICEServer>(in.ice_servers());
    out._subscriberPrimary = in.subscriber_primary();
    out._alternativeUrl = in.alternative_url();
    if (in.has_client_configuration()) {
        out._clientConfiguration = map(in.client_configuration());
    }
    out._serverRegion = in.server_region();
    out._pingTimeout = in.ping_timeout();
    out._pingInterval = in.ping_interval();
    if (in.has_server_info()) {
        out._serverInfo = map(in.server_info());
    }
    out._sifTrailer = in.sif_trailer();
    out._enabledPublishCodecs = map<Codec, livekit::Codec>(in.enabled_publish_codecs());
    out._fastPublish = in.fast_publish();
    return out;
}

SessionDescription Signals::map(const livekit::SessionDescription& in)
{
    SessionDescription out;
    out._type = in.type();
    out._sdp = in.sdp();
    return out;
}

livekit::SessionDescription Signals::map(const SessionDescription& in)
{
    livekit::SessionDescription out;
    out.set_type(in._type);
    out.set_sdp(in._sdp);
    return out;
}

TrickleRequest Signals::map(const livekit::TrickleRequest& in)
{
    TrickleRequest out;
    out._candidateInit = in.candidateinit();
    out._target = map(in.target());
    out._final = in.final();
    return out;
}

livekit::TrickleRequest Signals::map(const TrickleRequest& in)
{
    livekit::TrickleRequest out;
    out.set_candidateinit(in._candidateInit);
    out.set_target(map(in._target));
    out.set_final(in._final);
    return out;
}

ParticipantUpdate Signals::map(const livekit::ParticipantUpdate& in)
{
    ParticipantUpdate out;
    out._participants = map<ParticipantInfo, livekit::ParticipantInfo>(in.participants());
    return out;
}

TrackPublishedResponse Signals::map(const livekit::TrackPublishedResponse& in)
{
    TrackPublishedResponse out;
    out._cid = in.cid();
    out._track = map(in.track());
    return out;
}

TrackUnpublishedResponse Signals::map(const livekit::TrackUnpublishedResponse& in)
{
    TrackUnpublishedResponse out;
    out._trackSid = in.track_sid();
    return out;
}

LeaveRequest Signals::map(const livekit::LeaveRequest& in)
{
    LeaveRequest out;
    out._canReconnect = in.can_reconnect();
    out._reason = map(in.reason());
    out._action = map(in.action());
    out._regions = map(in.regions());
    return out;
}

MuteTrackRequest Signals::map(const livekit::MuteTrackRequest& in)
{
    MuteTrackRequest out;
    out._sid = in.sid();
    out._muted = in.muted();
    return out;
}

livekit::MuteTrackRequest Signals::map(const MuteTrackRequest& in)
{
    livekit::MuteTrackRequest out;
    out.set_sid(in._sid);
    out.set_muted(in._muted);
    return out;
}

SpeakersChanged Signals::map(const livekit::SpeakersChanged& in)
{
    SpeakersChanged out;
    out._speakers = map<SpeakerInfo, livekit::SpeakerInfo>(in.speakers());
    return out;
}

RoomUpdate Signals::map(const livekit::RoomUpdate& in)
{
    RoomUpdate out;
    if (in.has_room()) {
        out._room = map(in.room());
    }
    return out;
}

ConnectionQualityUpdate Signals::map(const livekit::ConnectionQualityUpdate& in)
{
    ConnectionQualityUpdate out;
    out._updates = map<ConnectionQualityInfo, livekit::ConnectionQualityInfo>(in.updates());
    return out;
}

StreamStateUpdate Signals::map(const livekit::StreamStateUpdate& in)
{
    StreamStateUpdate out;
    out._streamStates = map<StreamStateInfo, livekit::StreamStateInfo>(in.stream_states());
    return out;
}

SubscribedQualityUpdate Signals::map(const livekit::SubscribedQualityUpdate& in)
{
    SubscribedQualityUpdate out;
    out._trackSid = in.track_sid();
    out._subscribedQualities = map<SubscribedQuality, livekit::SubscribedQuality>(in.subscribed_qualities());
    out._subscribedCodecs = map<SubscribedCodec, livekit::SubscribedCodec>(in.subscribed_codecs());
    return out;
}

ReconnectResponse Signals::map(const livekit::ReconnectResponse& in)
{
    ReconnectResponse out;
    out._iceServers = map<ICEServer, livekit::ICEServer>(in.ice_servers());
    if (in.has_client_configuration()) {
        out._clientConfiguration = map(in.client_configuration());
    }
    return out;
}

TrackSubscribed Signals::map(const livekit::TrackSubscribed& in)
{
    TrackSubscribed out;
    out._trackSid = in.track_sid();
    return out;
}

RequestResponse Signals::map(const livekit::RequestResponse& in)
{
    RequestResponse out;
    out._requestId = in.request_id();
    out._reason = map(in.reason());
    out._message = in.message();
    return out;
}

SubscriptionResponse Signals::map(const livekit::SubscriptionResponse& in)
{
    SubscriptionResponse out;
    out._trackSid = in.track_sid();
    out._err = map(in.err());
    return out;
}

SubscriptionPermissionUpdate Signals::map(const livekit::SubscriptionPermissionUpdate& in)
{
    SubscriptionPermissionUpdate out;
    out._participantSid = in.participant_sid();
    out._trackSid = in.track_sid();
    out._allowed = in.allowed();
    return out;
}

AddTrackRequest Signals::map(const livekit::AddTrackRequest& in)
{
    AddTrackRequest out;
    out._cid = in.cid();
    out._name = in.name();
    out._type = map(in.type());
    out._width = in.width();
    out._height = in.height();
    out._muted = in.muted();
    out._disableDtx = in.disable_dtx();
    out._source = map(in.source());
    out._layers = map<VideoLayer, livekit::VideoLayer>(in.layers());
    out._simulcastCodecs = map<SimulcastCodec, livekit::SimulcastCodec>(in.simulcast_codecs());
    out._sid = in.sid();
    out._stereo = in.stereo();
    out._disableRed = in.disable_red();
    out._encryption = map(in.encryption());
    out._stream = in.stream();
    out._backupCodecPolicy = map(in.backup_codec_policy());
    return out;
}

livekit::AddTrackRequest Signals::map(const AddTrackRequest& in)
{
    livekit::AddTrackRequest out;
    out.set_cid(in._cid);
    out.set_name(in._name);
    out.set_type(map(in._type));
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_muted(in._muted);
    out.set_disable_dtx(in._disableDtx);
    out.set_source(map(in._source));
    map(in._layers, out.mutable_layers());
    map(in._simulcastCodecs, out.mutable_simulcast_codecs());
    out.set_sid(in._sid);
    out.set_stereo(in._stereo);
    out.set_disable_red(in._disableRed);
    out.set_encryption(map(in._encryption));
    out.set_stream(in._stream);
    out.set_backup_codec_policy(map(in._backupCodecPolicy));
    return out;
}

Room Signals::map(const livekit::Room& in)
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
    out._enabledCodecs = map<Codec, livekit::Codec>(in.enabled_codecs());
    out._metadata = in.metadata();
    out._numParticipants = in.num_participants();
    out._numPublishers = in.num_publishers();
    out._activeRecording = in.active_recording();
    if (in.has_version()) {
        out._version = map(in.version());
    }
    return out;
}

Codec Signals::map(const livekit::Codec& in)
{
    return {in.mime(), in.fmtp_line()};
}

TimedVersion Signals::map(const livekit::TimedVersion& in)
{
    return {in.unix_micro(), in.ticks()};
}

ParticipantInfo Signals::map(const livekit::ParticipantInfo& in)
{
    ParticipantInfo out;
    out._sid = in.sid();
    out._identity = in.identity();
    out._state = map(in.state());
    out._tracks = map<TrackInfo, livekit::TrackInfo>(in.tracks());
    out._metadata = in.metadata();
    out._joinedAt = in.joined_at();
    out.joinedAtMs = in.joined_at_ms();
    out._name = in.name();
    out._version = in.version();
    if (in.has_permission()) {
        out._permission = map(in.permission());
    }
    out._region = in.region();
    out._isPublisher = in.is_publisher();
    out._kind = map(in.kind());
    out._attributes = map(in.attributes());
    out._disconnectReason = map(in.disconnect_reason());
    return out;
}

ParticipantKind Signals::map(livekit::ParticipantInfo_Kind in)
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

ParticipantState Signals::map(livekit::ParticipantInfo_State in)
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

ParticipantPermission Signals::map(const livekit::ParticipantPermission& in)
{
    ParticipantPermission out;
    out._canSubscribe = in.can_subscribe();
    out._canPublish = in.can_publish();
    out._canPublish_data = in.can_publish_data();
    out._canPublishSources = map<TrackSource, livekit::TrackSource>(in.can_publish_sources());
    out._hidden = in.hidden();
    out._recorder = in.recorder();
    out._canUpdateMetadata = in.can_update_metadata();
    out._agent = in.agent();
    out._canSubscribeMetrics = in.can_subscribe_metrics();
    return out;
}

DisconnectReason Signals::map(livekit::DisconnectReason in)
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

TrackSource Signals::map(livekit::TrackSource in)
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

livekit::TrackSource Signals::map(TrackSource in)
{
    switch (in) {
        case TrackSource::Unknown:
            break;
        case TrackSource::Camera:
            return livekit::CAMERA;
        case TrackSource::Microphone:
            return livekit::MICROPHONE;
        case TrackSource::ScreenShare:
            return livekit::SCREEN_SHARE;
        case TrackSource::ScreenShareAudio:
            return livekit::SCREEN_SHARE_AUDIO;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::UNKNOWN;
}

TrackInfo Signals::map(const livekit::TrackInfo& in)
{
    TrackInfo out;
    out._sid = in.sid();
    out._type = map(in.type());
    out._name = in.name();
    out._muted = in.muted();
    out._width = in.width();
    out._height = in.height();
    out._simulcast = in.simulcast();
    out._disableDtx = in.disable_dtx();
    out._source = map(in.source());
    out._layers = map<VideoLayer, livekit::VideoLayer>(in.layers());
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    out._codecs = map<SimulcastCodecInfo, livekit::SimulcastCodecInfo>(in.codecs());
    out._stereo = in.stereo();
    out._disableRed = in.disable_red();
    out._encryption = map(in.encryption());
    out._stream = in.stream();
    if (in.has_version()) {
        out._version = map(in.version());
    }
    out._audioFeatures = map<AudioTrackFeature, livekit::AudioTrackFeature>(in.audio_features());
    out._backupCodecPolicy = map(in.backup_codec_policy());
    return out;
}

VideoQuality Signals::map(livekit::VideoQuality in)
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

livekit::VideoQuality Signals::map(VideoQuality in)
{
    switch (in) {
        case VideoQuality::Low:
            break;
        case VideoQuality::Medium:
            return livekit::MEDIUM;
        case VideoQuality::High:
            return livekit::HIGH;
        case VideoQuality::Off:
            return livekit::OFF;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::LOW;
}

VideoLayer Signals::map(const livekit::VideoLayer& in)
{
    VideoLayer out;
    out._quality = map(in.quality());
    out._width = in.width();
    out._height = in.height();
    out._bitrate = in.bitrate();
    out._ssrc = in.ssrc();
    return out;
}

livekit::VideoLayer Signals::map(const VideoLayer& in)
{
    livekit::VideoLayer out;
    out.set_quality(map(in._quality));
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_bitrate(in._bitrate);
    out.set_ssrc(in._ssrc);
    return out;
}

TrackType Signals::map(livekit::TrackType in)
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

livekit::TrackType Signals::map(TrackType in)
{
    switch (in) {
        case TrackType::Audio:
            break;
        case TrackType::Video:
            return livekit::VIDEO;
        case TrackType::Data:
            return livekit::DATA;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::AUDIO;
}

SimulcastCodecInfo Signals::map(const livekit::SimulcastCodecInfo& in)
{
    SimulcastCodecInfo out;
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    out._cid = in.cid();
    out._layers = map<VideoLayer, livekit::VideoLayer>(in.layers());
    return out;
}

BackupCodecPolicy Signals::map(livekit::BackupCodecPolicy in)
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

livekit::BackupCodecPolicy Signals::map(BackupCodecPolicy in)
{
    switch (in) {
        case BackupCodecPolicy::Regression:
            break;
        case BackupCodecPolicy::Simulcast:
            return livekit::SIMULCAST;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::REGRESSION;
}

EncryptionType Signals::map(livekit::Encryption_Type in)
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

livekit::Encryption_Type Signals::map(EncryptionType in)
{
    switch (in) {
        case EncryptionType::None:
            break;
        case EncryptionType::Gcm:
            return livekit::Encryption_Type_GCM;
        case EncryptionType::Custom:
            return livekit::Encryption_Type_CUSTOM;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::Encryption_Type_NONE;
}

AudioTrackFeature Signals::map(livekit::AudioTrackFeature in)
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

ClientConfigSetting Signals::map(livekit::ClientConfigSetting in)
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

ClientConfiguration Signals::map(const livekit::ClientConfiguration& in)
{
    ClientConfiguration out;
    out._video = map(in.video());
    out._screen = map(in.screen());
    out._resumeConnection = map(in.resume_connection());
    out._disabledCodecs = map(in.disabled_codecs());
    out._forceRelay = map(in.force_relay());
    return out;
}

DisabledCodecs Signals::map(const livekit::DisabledCodecs& in)
{
    DisabledCodecs out;
    out._codecs = map<Codec, livekit::Codec>(in.codecs());
    out._publish = map<Codec, livekit::Codec>(in.publish());
    return out;
}

VideoConfiguration Signals::map(const livekit::VideoConfiguration& in)
{
    VideoConfiguration out;
    out._hardwareEncoder = map(in.hardware_encoder());
    return out;
}

ServerEdition Signals::map(livekit::ServerInfo_Edition in)
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

ServerInfo Signals::map(const livekit::ServerInfo& in)
{
    ServerInfo out;
    out._edition = map(in.edition());
    out._version = in.version();
    out._protocol = in.protocol();
    out._region = in.region();
    out._nodeId = in.node_id();
    out._debugInfo = in.debug_info();
    out._agentProtocol = in.agent_protocol();
    return out;
}

SignalTarget Signals::map(livekit::SignalTarget in)
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

livekit::SignalTarget Signals::map(SignalTarget in)
{
    switch (in) {
        case SignalTarget::Publisher:
            break;
        case SignalTarget::Subscriber:
            return livekit::SUBSCRIBER;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::PUBLISHER;
}

LeaveRequestAction Signals::map(livekit::LeaveRequest_Action in)
{
    switch (in) {
        case livekit::LeaveRequest_Action_DISCONNECT:
            break;
        case livekit::LeaveRequest_Action_RESUME:
            return LeaveRequestAction::Resume;
        case livekit::LeaveRequest_Action_RECONNECT:
            return LeaveRequestAction::Reconnect;
        default: // TODO: log error
            assert(false);
            break;
    }
    return LeaveRequestAction::Disconnect;
}

RegionInfo Signals::map(const livekit::RegionInfo& in)
{
    RegionInfo out;
    out._region = in.region();
    out._url = in.url();
    out._distance = in.distance();
    return out;
}

RegionSettings Signals::map(const livekit::RegionSettings& in)
{
    RegionSettings out;
    out._regions = map<RegionInfo, livekit::RegionInfo>(in.regions());
    return out;
}

SpeakerInfo Signals::map(const livekit::SpeakerInfo& in)
{
    SpeakerInfo out;
    out._sid = in.sid();
    out._level = in.level();
    out._active = in.active();
    return out;
}

ConnectionQuality Signals::map(livekit::ConnectionQuality in)
{
    switch (in) {
        case livekit::POOR:
            break;
        case livekit::GOOD:
            return ConnectionQuality::Good;
        case livekit::EXCELLENT:
            return ConnectionQuality::Excellent;
        case livekit::LOST:
            return ConnectionQuality::Lost;
        default: // TODO: log error
            assert(false);
            break;
    }
    return ConnectionQuality::Poor;
}

ConnectionQualityInfo Signals::map(const livekit::ConnectionQualityInfo& in)
{
    ConnectionQualityInfo out;
    out._participantSid = in.participant_sid();
    out._quality = map(in.quality());
    out._score = in.score();
    return out;
}

StreamState Signals::map(livekit::StreamState in)
{
    switch (in) {
        case livekit::ACTIVE:
            break;
        case livekit::PAUSED:
            return StreamState::Paused;
        default: // TODO: log error
            assert(false);
            break;
    }
    return StreamState::Active;
}

StreamStateInfo Signals::map(const livekit::StreamStateInfo& in)
{
    StreamStateInfo out;
    out._participantSid = in.participant_sid();
    out._trackSid = in.track_sid();
    out._state = map(in.state());
    return out;
}

SubscribedQuality Signals::map(const livekit::SubscribedQuality& in)
{
    SubscribedQuality out;
    out._quality = map(in.quality());
    out._enabled = in.enabled();
    return out;
}

SubscribedCodec Signals::map(const livekit::SubscribedCodec& in)
{
    SubscribedCodec out;
    out._codec = in.codec();
    out._qualities = map<SubscribedQuality, livekit::SubscribedQuality>(in.qualities());
    return out;
}

ICEServer Signals::map(const livekit::ICEServer& in)
{
    ICEServer out;
    out._urls.reserve(in.urls_size());
    for (const auto& url : in.urls()) {
        out._urls.push_back(url);
    }
    out._username = in.username();
    out._credential = in.credential();
    return out;
}

RequestResponseReason Signals::map(livekit::RequestResponse_Reason in)
{
    switch (in) {
        case livekit::RequestResponse_Reason_OK:
            break;
        case livekit::RequestResponse_Reason_NOT_FOUND:
            return RequestResponseReason::NotFound;
        case livekit::RequestResponse_Reason_NOT_ALLOWED:
            return RequestResponseReason::NotAllowed;
        case livekit::RequestResponse_Reason_LIMIT_EXCEEDED:
            return RequestResponseReason::LimitExceeded;
        default: // TODO: log error
            assert(false);
            break;
    }
    return RequestResponseReason::Ok;
}

SubscriptionError Signals::map(livekit::SubscriptionError in)
{
    switch (in) {
        case livekit::SE_UNKNOWN:
            break;
        case livekit::SE_CODEC_UNSUPPORTED:
            return SubscriptionError::CodecUnsupported;
        case livekit::SE_TRACK_NOTFOUND:
            return SubscriptionError::TrackNotfound;
        default: // TODO: log error
            assert(false);
            break;
    }
    return SubscriptionError::Unknown;
}

SimulcastCodec Signals::map(const livekit::SimulcastCodec& in)
{
    SimulcastCodec out;
    out._cid = in.cid();
    out._codec = in.codec();
    return out;
}

livekit::SimulcastCodec Signals::map(const SimulcastCodec& in)
{
    livekit::SimulcastCodec out;
    out.set_cid(in._cid);
    out.set_codec(in._codec);
    return out;
}

template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
std::vector<TCppType> Signals::map(const TProtoBufRepeated& in)
{
    if (const auto size = in.size()) {
        std::vector<TCppType> out;
        out.reserve(size_t(size));
        for (const auto& val : in) {
            out.push_back(map(TProtoBufType(val)));
        }
        return out;
    }
    return {};
}

template <typename TCppRepeated, class TProtoBufRepeated>
void Signals::map(const TCppRepeated& from, TProtoBufRepeated* to)
{
    if (to) {
        to->Reserve(int(to->size() + from.size()));
        for (const auto& val : from) {
            *to->Add() = map(val);
        }
    }
}

template<typename K, typename V>
std::unordered_map<K, V> Signals::map(const google::protobuf::Map<K, V>& in)
{
    if (const auto size = in.size()) {
        std::unordered_map<K, V> out;
        out.reserve(size);
        for (auto it = in.begin(); it != in.end(); ++it) {
            out[it->first] = it->second;
        }
        return out;
    }
    return {};
}

} // namespace LiveKitCpp
