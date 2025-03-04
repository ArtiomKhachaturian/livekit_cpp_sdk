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
    out._otherParticipants = rconv<ParticipantInfo, livekit::ParticipantInfo>(in.other_participants());
    out._serverVersion = in.server_version();
    out._iceServers = rconv<ICEServer, livekit::ICEServer>(in.ice_servers());
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
    out._enabledPublishCodecs = rconv<Codec, livekit::Codec>(in.enabled_publish_codecs());
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
    out._participants = rconv<ParticipantInfo, livekit::ParticipantInfo>(in.participants());
    return out;
}

TrackPublishedResponse Signals::map(const livekit::TrackPublishedResponse& in)
{
    TrackPublishedResponse out;
    out._cid = in.cid();
    out._track = map(in.track());
    return out;
}

livekit::TrackPublishedResponse Signals::map(const TrackPublishedResponse& in)
{
    livekit::TrackPublishedResponse out;
    out.set_cid(in._cid);
    *out.mutable_track() = map(in._track);
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

livekit::LeaveRequest Signals::map(const LeaveRequest& in)
{
    livekit::LeaveRequest out;
    out.set_can_reconnect(in._canReconnect);
    out.set_reason(map(in._reason));
    out.set_action(map(in._action));
    *out.mutable_regions() = map(in._regions);
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
    out._speakers = rconv<SpeakerInfo, livekit::SpeakerInfo>(in.speakers());
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
    out._updates = rconv<ConnectionQualityInfo, livekit::ConnectionQualityInfo>(in.updates());
    return out;
}

StreamStateUpdate Signals::map(const livekit::StreamStateUpdate& in)
{
    StreamStateUpdate out;
    out._streamStates = rconv<StreamStateInfo, livekit::StreamStateInfo>(in.stream_states());
    return out;
}

SubscribedQualityUpdate Signals::map(const livekit::SubscribedQualityUpdate& in)
{
    SubscribedQualityUpdate out;
    out._trackSid = in.track_sid();
    out._subscribedQualities = rconv<SubscribedQuality, livekit::SubscribedQuality>(in.subscribed_qualities());
    out._subscribedCodecs = rconv<SubscribedCodec, livekit::SubscribedCodec>(in.subscribed_codecs());
    return out;
}

ReconnectResponse Signals::map(const livekit::ReconnectResponse& in)
{
    ReconnectResponse out;
    out._iceServers = rconv<ICEServer, livekit::ICEServer>(in.ice_servers());
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
    out._layers = rconv<VideoLayer, livekit::VideoLayer>(in.layers());
    out._simulcastCodecs = rconv<SimulcastCodec, livekit::SimulcastCodec>(in.simulcast_codecs());
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
    rconv(in._layers, out.mutable_layers());
    rconv(in._simulcastCodecs, out.mutable_simulcast_codecs());
    out.set_sid(in._sid);
    out.set_stereo(in._stereo);
    out.set_disable_red(in._disableRed);
    out.set_encryption(map(in._encryption));
    out.set_stream(in._stream);
    out.set_backup_codec_policy(map(in._backupCodecPolicy));
    return out;
}

UpdateSubscription Signals::map(const livekit::UpdateSubscription& in)
{
    UpdateSubscription out;
    out._trackSids = rconv<std::string>(in.track_sids());
    out._subscribe = in.subscribe();
    out._participantTracks = rconv<ParticipantTracks, livekit::ParticipantTracks>(in.participant_tracks());
    return out;
}

livekit::UpdateSubscription Signals::map(const UpdateSubscription& in)
{
    livekit::UpdateSubscription out;
    rconv(in._trackSids, out.mutable_track_sids());
    out.set_subscribe(in._subscribe);
    rconv(in._participantTracks, out.mutable_participant_tracks());
    return out;
}

UpdateTrackSettings Signals::map(const livekit::UpdateTrackSettings& in)
{
    UpdateTrackSettings out;
    out._trackSids = rconv<std::string>(in.track_sids());
    out._disabled = in.disabled();
    out._quality = map(in.quality());
    out._width = in.width();
    out._height = in.height();
    out._fps = in.fps();
    out._priority = in.priority();
    return out;
}

livekit::UpdateTrackSettings Signals::map(const UpdateTrackSettings& in)
{
    livekit::UpdateTrackSettings out;
    rconv(in._trackSids, out.mutable_track_sids());
    out.set_disabled(in._disabled);
    out.set_quality(map(in._quality));
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_fps(in._fps);
    out.set_priority(in._priority);
    return out;
}

UpdateVideoLayers Signals::map(const livekit::UpdateVideoLayers& in)
{
    UpdateVideoLayers out;
    out._trackSid = in.track_sid();
    out._layers = rconv<VideoLayer, livekit::VideoLayer>(in.layers());
    return out;
}

livekit::UpdateVideoLayers Signals::map(const UpdateVideoLayers& in)
{
    livekit::UpdateVideoLayers out;
    out.set_track_sid(in._trackSid);
    rconv(in._layers, out.mutable_layers());
    return out;
}

SubscriptionPermission Signals::map(const livekit::SubscriptionPermission& in)
{
    SubscriptionPermission out;
    out._allParticipants = in.all_participants();
    out._trackPermissions = rconv<TrackPermission, livekit::TrackPermission>(in.track_permissions());
    return out;
}

livekit::SubscriptionPermission Signals::map(const SubscriptionPermission& in)
{
    livekit::SubscriptionPermission out;
    out.set_all_participants(in._allParticipants);
    rconv(in._trackPermissions, out.mutable_track_permissions());
    return out;
}

SyncState Signals::map(const livekit::SyncState& in)
{
    SyncState out;
    out._answer = map(in.answer());
    out._subscription = map(in.subscription());
    out._publishTracks = rconv<TrackPublishedResponse, livekit::TrackPublishedResponse>(in.publish_tracks());
    out._dataChannels = rconv<DataChannelInfo, livekit::DataChannelInfo>(in.data_channels());
    out._offer = map(in.offer());
    out._trackSidsDisabled = rconv<std::string>(in.track_sids_disabled());
    return out;
}

livekit::SyncState Signals::map(const SyncState& in)
{
    livekit::SyncState out;
    *out.mutable_answer() = map(in._answer);
    *out.mutable_subscription() = map(in._subscription);
    rconv(in._publishTracks, out.mutable_publish_tracks());
    rconv(in._dataChannels, out.mutable_data_channels());
    *out.mutable_offer() = map(in._offer);
    rconv(in._trackSidsDisabled, out.mutable_track_sids_disabled());
    return out;
}

SimulateScenario Signals::map(const livekit::SimulateScenario& in)
{
    SimulateScenario out;
    switch (in.scenario_case()) {
        case livekit::SimulateScenario::kSpeakerUpdate:
            out._case = SimulateScenario::Case::SpeakerUpdate;
            out._scenario._speakerUpdate = in.speaker_update();
            break;
        case livekit::SimulateScenario::kNodeFailure:
            out._case = SimulateScenario::Case::NodeFailure;
            out._scenario._nodeFailure = in.node_failure();
            break;
        case livekit::SimulateScenario::kMigration:
            out._case = SimulateScenario::Case::Migration;
            out._scenario._migration = in.migration();
            break;
        case livekit::SimulateScenario::kServerLeave:
            out._case = SimulateScenario::Case::ServerLeave;
            out._scenario._serverLeave = in.server_leave();
            break;
        case livekit::SimulateScenario::kSwitchCandidateProtocol:
            out._case = SimulateScenario::Case::SwitchCandidateProtocol;
            out._scenario._switchCandidateProtocol = map(in.switch_candidate_protocol());
            break;
        case livekit::SimulateScenario::kSubscriberBandwidth:
            out._case = SimulateScenario::Case::SubscriberBandwidth;
            out._scenario._subscriberBandwidth = in.subscriber_bandwidth();
            break;
        case livekit::SimulateScenario::kDisconnectSignalOnResume:
            out._case = SimulateScenario::Case::DisconnectSignalOnResume;
            out._scenario._disconnectSignalOnResume = in.disconnect_signal_on_resume();
            break;
        case livekit::SimulateScenario::kDisconnectSignalOnResumeNoMessages:
            out._case = SimulateScenario::Case::DisconnectSignalOnResumeNoMessages;
            out._scenario._disconnectSignalOnResumeNoMessages = in.disconnect_signal_on_resume_no_messages();
            break;
        case livekit::SimulateScenario::kLeaveRequestFullReconnect:
            out._case = SimulateScenario::Case::LeaveRequestFullReconnect;
            out._scenario._leaveRequestFullReconnect = in.leave_request_full_reconnect();
            break;
        case livekit::SimulateScenario::SCENARIO_NOT_SET:
            break;
        default: // TODO: log error
            assert(false);
            break;
    }
    return out;
}

livekit::SimulateScenario Signals::map(const SimulateScenario& in)
{
    livekit::SimulateScenario out;
    switch (in._case) {
        case SimulateScenario::Case::NotSet:
            break;
        case SimulateScenario::Case::SpeakerUpdate:
            out.set_speaker_update(in._scenario._speakerUpdate);
            break;
        case SimulateScenario::Case::NodeFailure:
            out.set_node_failure(in._scenario._nodeFailure);
            break;
        case SimulateScenario::Case::Migration:
            out.set_migration(in._scenario._migration);
            break;
        case SimulateScenario::Case::ServerLeave:
            out.set_server_leave(in._scenario._serverLeave);
            break;
        case SimulateScenario::Case::SwitchCandidateProtocol:
            out.set_switch_candidate_protocol(map(in._scenario._switchCandidateProtocol));
            break;
        case SimulateScenario::Case::SubscriberBandwidth:
            out.set_subscriber_bandwidth(in._scenario._subscriberBandwidth);
            break;
        case SimulateScenario::Case::DisconnectSignalOnResume:
            out.set_disconnect_signal_on_resume(in._scenario._disconnectSignalOnResume);
            break;
        case SimulateScenario::Case::DisconnectSignalOnResumeNoMessages:
            out.set_disconnect_signal_on_resume_no_messages(in._scenario._disconnectSignalOnResumeNoMessages);
            break;
        case SimulateScenario::Case::LeaveRequestFullReconnect:
            out.set_leave_request_full_reconnect(in._scenario._leaveRequestFullReconnect);
            break;
        default: // TODO: log error
            assert(false);
            break;
    }
    return out;
}

UpdateParticipantMetadata Signals::map(const livekit::UpdateParticipantMetadata& in)
{
    UpdateParticipantMetadata out;
    out._metadata = in.metadata();
    out._name = in.name();
    out._attributes = mconv(in.attributes());
    out._requestId = in.request_id();
    return out;
}

livekit::UpdateParticipantMetadata Signals::map(const UpdateParticipantMetadata& in)
{
    livekit::UpdateParticipantMetadata out;
    out.set_metadata(in._metadata);
    out.set_name(in._name);
    mconv(in._attributes, out.mutable_attributes());
    out.set_request_id(in._requestId);
    return out;
}

Ping Signals::map(const livekit::Ping& in)
{
    Ping out;
    out._timestamp = in.timestamp();
    out._rtt = in.rtt();
    return out;
}

livekit::Ping Signals::map(const Ping& in)
{
    livekit::Ping out;
    out.set_timestamp(in._timestamp);
    out.set_rtt(in._rtt);
    return out;
}

UpdateLocalAudioTrack Signals::map(const livekit::UpdateLocalAudioTrack& in)
{
    UpdateLocalAudioTrack out;
    out._trackSid = in.track_sid();
    out._features = rconv<AudioTrackFeature, livekit::AudioTrackFeature>(in.features());
    return out;
}

livekit::UpdateLocalAudioTrack Signals::map(const UpdateLocalAudioTrack& in)
{
    livekit::UpdateLocalAudioTrack out;
    out.set_track_sid(in._trackSid);
    rconv(in._features, out.mutable_features());
    return out;
}

UpdateLocalVideoTrack Signals::map(const livekit::UpdateLocalVideoTrack& in)
{
    UpdateLocalVideoTrack out;
    out._trackSid = in.track_sid();
    out._width = in.width();
    out._height = in.height();
    return out;
}

livekit::UpdateLocalVideoTrack Signals::map(const UpdateLocalVideoTrack& in)
{
    livekit::UpdateLocalVideoTrack out;
    out.set_track_sid(in._trackSid);
    out.set_width(in._width);
    out.set_height(in._height);
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
    out._enabledCodecs = rconv<Codec, livekit::Codec>(in.enabled_codecs());
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

livekit::TimedVersion Signals::map(const TimedVersion& in)
{
    livekit::TimedVersion out;
    out.set_unix_micro(in._unixMicro);
    out.set_ticks(in._ticks);
    return out;
}

ParticipantInfo Signals::map(const livekit::ParticipantInfo& in)
{
    ParticipantInfo out;
    out._sid = in.sid();
    out._identity = in.identity();
    out._state = map(in.state());
    out._tracks = rconv<TrackInfo, livekit::TrackInfo>(in.tracks());
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
    out._attributes = mconv(in.attributes());
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
    out._canPublishSources = rconv<TrackSource, livekit::TrackSource>(in.can_publish_sources());
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

livekit::DisconnectReason Signals::map(DisconnectReason in)
{
    switch (in) {
        case DisconnectReason::UnknownReason:
            break;
        case DisconnectReason::ClientInitiated:
            return livekit::CLIENT_INITIATED;
        case DisconnectReason::DuplicateIdentity:
            return livekit::DUPLICATE_IDENTITY;
        case DisconnectReason::ServerShutdown:
            return livekit::SERVER_SHUTDOWN;
        case DisconnectReason::ParticipantRemoved:
            return livekit::PARTICIPANT_REMOVED;
        case DisconnectReason::RoomDeleted:
            return livekit::ROOM_DELETED;
        case DisconnectReason::StateMismatch:
            return livekit::STATE_MISMATCH;
        case DisconnectReason::JoinFailure:
            return livekit::JOIN_FAILURE;
        case DisconnectReason::Migration:
            return livekit::MIGRATION;
        case DisconnectReason::SignalClose:
            return livekit::SIGNAL_CLOSE;
        case DisconnectReason::RoomClosed:
            return livekit::ROOM_CLOSED;
        case DisconnectReason::UserUnavailable:
            return livekit::USER_UNAVAILABLE;
        case DisconnectReason::UserRejected:
            return livekit::USER_REJECTED;
        case DisconnectReason::SipTrunkFailure:
            return livekit::SIP_TRUNK_FAILURE;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::UNKNOWN_REASON;
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
    out._layers = rconv<VideoLayer, livekit::VideoLayer>(in.layers());
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    out._codecs = rconv<SimulcastCodecInfo, livekit::SimulcastCodecInfo>(in.codecs());
    out._stereo = in.stereo();
    out._disableRed = in.disable_red();
    out._encryption = map(in.encryption());
    out._stream = in.stream();
    if (in.has_version()) {
        out._version = map(in.version());
    }
    out._audioFeatures = rconv<AudioTrackFeature, livekit::AudioTrackFeature>(in.audio_features());
    out._backupCodecPolicy = map(in.backup_codec_policy());
    return out;
}

livekit::TrackInfo Signals::map(const TrackInfo& in)
{
    livekit::TrackInfo out;
    out.set_sid(in._sid);
    out.set_type(map(in._type));
    out.set_name(in._name);
    out.set_muted(in._muted);
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_simulcast(in._simulcast);
    out.set_disable_dtx(in._disableDtx);
    out.set_source(map(in._source));
    rconv(in._layers, out.mutable_layers());
    out.set_mime_type(in._mimeType);
    out.set_mid(in._mid);
    rconv(in._codecs, out.mutable_codecs());
    out.set_disable_red(in._disableRed);
    out.set_encryption(map(in._encryption));
    out.set_stream(in._stream);
    if (in._version.has_value()) {
        *out.mutable_version() = map(in._version.value());
    }
    rconv(in._audioFeatures, out.mutable_audio_features());
    out.set_backup_codec_policy(map(in._backupCodecPolicy));
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
    out._layers = rconv<VideoLayer, livekit::VideoLayer>(in.layers());
    return out;
}

livekit::SimulcastCodecInfo Signals::map(const SimulcastCodecInfo& in)
{
    livekit::SimulcastCodecInfo out;
    out.set_mime_type(in._mimeType);
    out.set_mid(in._mid);
    out.set_cid(in._cid);
    rconv(in._layers, out.mutable_layers());
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

livekit::AudioTrackFeature Signals::map(AudioTrackFeature in)
{
    switch (in) {
        case AudioTrackFeature::TFStereo:
            break;
        case AudioTrackFeature::TFNoDtx:
            return livekit::TF_NO_DTX;
        case AudioTrackFeature::TFAutoGainControl:
            return livekit::TF_AUTO_GAIN_CONTROL;
        case AudioTrackFeature::TFEchocancellation:
            return livekit::TF_ECHO_CANCELLATION;
        case AudioTrackFeature::TFNoiseSuppression:
            return livekit::TF_NOISE_SUPPRESSION;
        case AudioTrackFeature::TFEnhancedNoiseCancellation:
            return livekit::TF_ENHANCED_NOISE_CANCELLATION;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::TF_STEREO;
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
    out._codecs = rconv<Codec, livekit::Codec>(in.codecs());
    out._publish = rconv<Codec, livekit::Codec>(in.publish());
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

livekit::LeaveRequest_Action Signals::map(LeaveRequestAction in)
{
    switch (in) {
        case LeaveRequestAction::Disconnect:
            break;
        case LeaveRequestAction::Resume:
            return livekit::LeaveRequest_Action_RESUME;
        case LeaveRequestAction::Reconnect:
            return livekit::LeaveRequest_Action_RECONNECT;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::LeaveRequest_Action_DISCONNECT;
}

RegionInfo Signals::map(const livekit::RegionInfo& in)
{
    RegionInfo out;
    out._region = in.region();
    out._url = in.url();
    out._distance = in.distance();
    return out;
}

livekit::RegionInfo Signals::map(const RegionInfo& in)
{
    livekit::RegionInfo out;
    out.set_region(in._region);
    out.set_url(in._url);
    out.set_distance(in._distance);
    return out;
}

RegionSettings Signals::map(const livekit::RegionSettings& in)
{
    RegionSettings out;
    out._regions = rconv<RegionInfo, livekit::RegionInfo>(in.regions());
    return out;
}

livekit::RegionSettings Signals::map(const RegionSettings& in)
{
    livekit::RegionSettings out;
    rconv(in._regions, out.mutable_regions());
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
    out._qualities = rconv<SubscribedQuality, livekit::SubscribedQuality>(in.qualities());
    return out;
}

ICEServer Signals::map(const livekit::ICEServer& in)
{
    ICEServer out;
    out._urls = rconv<std::string>(in.urls());
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

ParticipantTracks Signals::map(const livekit::ParticipantTracks& in)
{
    ParticipantTracks out;
    out._participantSid = in.participant_sid();
    out._trackSids = rconv<std::string>(in.track_sids());
    return out;
}

livekit::ParticipantTracks Signals::map(const ParticipantTracks& in)
{
    livekit::ParticipantTracks out;
    out.set_participant_sid(in._participantSid);
    rconv(in._trackSids, out.mutable_track_sids());
    return out;
}

TrackPermission Signals::map(const livekit::TrackPermission& in)
{
    TrackPermission out;
    out._participantSid = in.participant_sid();
    out._allAracks = in.all_tracks();
    out._trackSids = rconv<std::string>(in.track_sids());
    out._participantIdentity = in.participant_identity();
    return out;
}

livekit::TrackPermission Signals::map(const TrackPermission& in)
{
    livekit::TrackPermission out;
    out.set_participant_sid(in._participantSid);
    out.set_all_tracks(in._allAracks);
    rconv(in._trackSids, out.mutable_track_sids());
    out.set_participant_identity(in._participantIdentity);
    return out;
}

DataChannelInfo Signals::map(const livekit::DataChannelInfo& in)
{
    DataChannelInfo out;
    out._label = in.label();
    out._id = in.id();
    out._target = map(in.target());
    return out;
}

livekit::DataChannelInfo Signals::map(const DataChannelInfo& in)
{
    livekit::DataChannelInfo out;
    out.set_label(in._label);
    out.set_id(in._id);
    out.set_target(map(in._target));
    return out;
}

CandidateProtocol Signals::map(livekit::CandidateProtocol in)
{
    switch (in) {
        case livekit::UDP:
            break;
        case livekit::TCP:
            return CandidateProtocol::Tcp;
        case livekit::TLS:
            return CandidateProtocol::Tls;
        default: // TODO: log error
            assert(false);
            break;
    }
    return CandidateProtocol::Udp;
}

livekit::CandidateProtocol Signals::map(CandidateProtocol in)
{
    switch (in) {
        case CandidateProtocol::Udp:
            break;
        case CandidateProtocol::Tcp:
            return livekit::TCP;
        case CandidateProtocol::Tls:
            return livekit::TLS;
        default: // TODO: log error
            assert(false);
            break;
    }
    return livekit::UDP;
}

template <typename TOut, typename TIn, class TProtoBufRepeated>
std::vector<TOut> Signals::rconv(const TProtoBufRepeated& in)
{
    if (const auto size = in.size()) {
        std::vector<TOut> out;
        out.reserve(size_t(size));
        for (const auto& val : in) {
            out.push_back(map(TIn(val)));
        }
        return out;
    }
    return {};
}

template <typename TCppRepeated, class TProtoBufRepeated>
void Signals::rconv(const TCppRepeated& from, TProtoBufRepeated* to)
{
    if (to) {
        to->Reserve(int(to->size() + from.size()));
        for (const auto& val : from) {
            *to->Add() = map(val);
        }
    }
}

template<typename K, typename V>
std::unordered_map<K, V> Signals::mconv(const google::protobuf::Map<K, V>& in)
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

template<typename K, typename V>
void Signals::mconv(const std::unordered_map<K, V>& from, google::protobuf::Map<K, V>* to)
{
    if (to) {
        for (auto it = from.begin(); it != from.end(); ++it) {
            to->insert({it->first, it->second});
        }
    }
}

} // namespace LiveKitCpp
