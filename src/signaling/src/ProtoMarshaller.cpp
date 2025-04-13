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
#include "ProtoMarshaller.h"
#include "ProtoUtils.h"
#include <nlohmann/json.hpp>

/*#if defined(_MSC_VER) && !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif*/

namespace  {

// TODO: add type name demangling
template <typename T>
inline std::string typeName() { return typeid(T).name(); }

}

#define LOG_ERROR(error) logError(error);
#define TYPE_CONVERSION_ERROR(TypeFrom, TypeTo) LOG_ERROR("Failed convert from '" \
    + typeName<TypeFrom>() + "' to '" + typeName<TypeTo>() + "'") \
    assert(false); \

namespace LiveKitCpp
{

ProtoMarshaller::ProtoMarshaller(Bricks::Logger* logger)
    : Bricks::LoggableR<>(logger)
{
}

JoinResponse ProtoMarshaller::map(livekit::JoinResponse in) const
{
    JoinResponse out;
    out._room = map(std::move(in.room()));
    out._participant = map(in.participant());
    out._otherParticipants = rconv<ParticipantInfo, livekit::ParticipantInfo>(in.other_participants());
    out._serverVersion = in.server_version();
    out._iceServers = rconv<ICEServer, livekit::ICEServer>(in.ice_servers());
    out._subscriberPrimary = in.subscriber_primary();
    out._alternativeUrl = in.alternative_url();
    out._clientConfiguration = map(in.client_configuration());
    out._serverRegion = in.server_region();
    out._pingTimeout = in.ping_timeout();
    out._pingInterval = in.ping_interval();
    out._serverInfo = map(in.server_info());
    out._sifTrailer = in.sif_trailer();
    out._enabledPublishCodecs = rconv<Codec, livekit::Codec>(in.enabled_publish_codecs());
    out._fastPublish = in.fast_publish();
    return out;
}

SessionDescription ProtoMarshaller::map(livekit::SessionDescription in) const
{
    SessionDescription out;
    out._type = in.type();
    out._sdp = in.sdp();
    return out;
}

livekit::SessionDescription ProtoMarshaller::map(SessionDescription in) const
{
    livekit::SessionDescription out;
    out.set_type(std::move(in._type));
    out.set_sdp(std::move(in._sdp));
    return out;
}

TrickleRequest ProtoMarshaller::map(livekit::TrickleRequest in) const
{
    TrickleRequest out;
    const auto candidateInit = in.candidateinit();
    if (!candidateInit.empty()) {
        try {
            auto json = nlohmann::json::parse(candidateInit);
            out._candidate._sdp = json["candidate"].get<std::string>();
            out._candidate._sdpMid = json["sdpMid"].get<std::string>();
            out._candidate._sdpMLineIndex = json["sdpMLineIndex"].get<int>();
            // usernameFragment (may contains null value)
            auto ufrag = json["usernameFragment"];
            if (!ufrag.is_null()) {
                out._candidate._usernameFragment = ufrag.get<std::string>();
            }
        }
        catch (const std::exception& e) {
            logError(e.what());
        }
    }
    out._target = map(in.target());
    out._final = in.final();
    return out;
}

livekit::TrickleRequest ProtoMarshaller::map(TrickleRequest in) const
{
    livekit::TrickleRequest out;
    nlohmann::json candidateInit;
    candidateInit["candidate"] = in._candidate._sdp;
    candidateInit["sdpMid"] = in._candidate._sdpMid;
    candidateInit["sdpMLineIndex"] = in._candidate._sdpMLineIndex;
    if (in._candidate._usernameFragment.empty()) {
        candidateInit["usernameFragment"] = nullptr;
    }
    else {
        candidateInit["usernameFragment"] = in._candidate._usernameFragment;
    }
    out.set_candidateinit(nlohmann::to_string(candidateInit));
    out.set_target(map(in._target));
    out.set_final(in._final);
    return out;
}

ParticipantUpdate ProtoMarshaller::map(livekit::ParticipantUpdate in) const
{
    ParticipantUpdate out;
    out._participants = rconv<ParticipantInfo, livekit::ParticipantInfo>(in.participants());
    return out;
}

TrackPublishedResponse ProtoMarshaller::map(livekit::TrackPublishedResponse in) const
{
    TrackPublishedResponse out;
    out._cid = in.cid();
    out._track = map(in.track());
    return out;
}

livekit::TrackPublishedResponse ProtoMarshaller::map(TrackPublishedResponse in) const
{
    livekit::TrackPublishedResponse out;
    out.set_cid(std::move(in._cid));
    *out.mutable_track() = map(std::move(in._track));
    return out;
}

TrackUnpublishedResponse ProtoMarshaller::map(livekit::TrackUnpublishedResponse in) const
{
    TrackUnpublishedResponse out;
    out._trackSid = in.track_sid();
    return out;
}

LeaveRequest ProtoMarshaller::map(livekit::LeaveRequest in) const
{
    LeaveRequest out;
    out._canReconnect = in.can_reconnect();
    out._reason = map(in.reason());
    out._action = map(in.action());
    out._regions = map(in.regions());
    return out;
}

livekit::LeaveRequest ProtoMarshaller::map(LeaveRequest in) const
{
    livekit::LeaveRequest out;
    out.set_can_reconnect(in._canReconnect);
    out.set_reason(map(in._reason));
    out.set_action(map(in._action));
    *out.mutable_regions() = map(std::move(in._regions));
    return out;
}

MuteTrackRequest ProtoMarshaller::map(livekit::MuteTrackRequest in) const
{
    MuteTrackRequest out;
    out._sid = in.sid();
    out._muted = in.muted();
    return out;
}

livekit::MuteTrackRequest ProtoMarshaller::map(MuteTrackRequest in) const
{
    livekit::MuteTrackRequest out;
    out.set_sid(std::move(in._sid));
    out.set_muted(in._muted);
    return out;
}

SpeakersChanged ProtoMarshaller::map(livekit::SpeakersChanged in) const
{
    SpeakersChanged out;
    out._speakers = rconv<SpeakerInfo, livekit::SpeakerInfo>(in.speakers());
    return out;
}

RoomUpdate ProtoMarshaller::map(livekit::RoomUpdate in) const
{
    RoomUpdate out;
    if (in.has_room()) {
        out._room = map(in.room());
    }
    return out;
}

ConnectionQualityUpdate ProtoMarshaller::map(livekit::ConnectionQualityUpdate in) const
{
    ConnectionQualityUpdate out;
    out._updates = rconv<ConnectionQualityInfo, livekit::ConnectionQualityInfo>(in.updates());
    return out;
}

StreamStateUpdate ProtoMarshaller::map(livekit::StreamStateUpdate in) const
{
    StreamStateUpdate out;
    out._streamStates = rconv<StreamStateInfo, livekit::StreamStateInfo>(in.stream_states());
    return out;
}

SubscribedQualityUpdate ProtoMarshaller::map(livekit::SubscribedQualityUpdate in) const
{
    SubscribedQualityUpdate out;
    out._trackSid = in.track_sid();
    out._subscribedQualities = rconv<SubscribedQuality, livekit::SubscribedQuality>(in.subscribed_qualities());
    out._subscribedCodecs = rconv<SubscribedCodec, livekit::SubscribedCodec>(in.subscribed_codecs());
    return out;
}

ReconnectResponse ProtoMarshaller::map(livekit::ReconnectResponse in) const
{
    ReconnectResponse out;
    out._iceServers = rconv<ICEServer, livekit::ICEServer>(in.ice_servers());
    if (in.has_client_configuration()) {
        out._clientConfiguration = map(in.client_configuration());
    }
    return out;
}

TrackSubscribed ProtoMarshaller::map(livekit::TrackSubscribed in) const
{
    TrackSubscribed out;
    out._trackSid = in.track_sid();
    return out;
}

RequestResponse ProtoMarshaller::map(livekit::RequestResponse in) const
{
    RequestResponse out;
    out._requestId = in.request_id();
    out._reason = map(in.reason());
    out._message = in.message();
    return out;
}

SubscriptionResponse ProtoMarshaller::map(livekit::SubscriptionResponse in) const
{
    SubscriptionResponse out;
    out._trackSid = in.track_sid();
    out._err = map(in.err());
    return out;
}

SubscriptionPermissionUpdate ProtoMarshaller::map(livekit::SubscriptionPermissionUpdate in) const
{
    SubscriptionPermissionUpdate out;
    out._participantSid = in.participant_sid();
    out._trackSid = in.track_sid();
    out._allowed = in.allowed();
    return out;
}

AddTrackRequest ProtoMarshaller::map(livekit::AddTrackRequest in) const
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

livekit::AddTrackRequest ProtoMarshaller::map(AddTrackRequest in) const
{
    livekit::AddTrackRequest out;
    out.set_cid(std::move(in._cid));
    out.set_name(std::move(in._name));
    out.set_type(map(in._type));
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_muted(in._muted);
    out.set_disable_dtx(in._disableDtx);
    out.set_source(map(in._source));
    rconv(std::move(in._layers), out.mutable_layers());
    rconv(std::move(in._simulcastCodecs), out.mutable_simulcast_codecs());
    out.set_sid(std::move(in._sid));
    out.set_stereo(in._stereo);
    out.set_disable_red(in._disableRed);
    out.set_encryption(map(in._encryption));
    out.set_stream(std::move(in._stream));
    out.set_backup_codec_policy(map(in._backupCodecPolicy));
    return out;
}

UpdateSubscription ProtoMarshaller::map(livekit::UpdateSubscription in) const
{
    UpdateSubscription out;
    out._trackSids = rconv<std::string>(in.track_sids());
    out._subscribe = in.subscribe();
    out._participantTracks = rconv<ParticipantTracks, livekit::ParticipantTracks>(in.participant_tracks());
    return out;
}

livekit::UpdateSubscription ProtoMarshaller::map(UpdateSubscription in) const
{
    livekit::UpdateSubscription out;
    rconv(std::move(in._trackSids), out.mutable_track_sids());
    out.set_subscribe(in._subscribe);
    rconv(std::move(in._participantTracks), out.mutable_participant_tracks());
    return out;
}

UpdateTrackSettings ProtoMarshaller::map(livekit::UpdateTrackSettings in) const
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

livekit::UpdateTrackSettings ProtoMarshaller::map(UpdateTrackSettings in) const
{
    livekit::UpdateTrackSettings out;
    rconv(std::move(in._trackSids), out.mutable_track_sids());
    out.set_disabled(in._disabled);
    out.set_quality(map(in._quality));
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_fps(in._fps);
    out.set_priority(in._priority);
    return out;
}

UpdateVideoLayers ProtoMarshaller::map(livekit::UpdateVideoLayers in) const
{
    UpdateVideoLayers out;
    out._trackSid = in.track_sid();
    out._layers = rconv<VideoLayer, livekit::VideoLayer>(in.layers());
    return out;
}

livekit::UpdateVideoLayers ProtoMarshaller::map(UpdateVideoLayers in) const
{
    livekit::UpdateVideoLayers out;
    out.set_track_sid(in._trackSid);
    rconv(in._layers, out.mutable_layers());
    return out;
}

SubscriptionPermission ProtoMarshaller::map(livekit::SubscriptionPermission in) const
{
    SubscriptionPermission out;
    out._allParticipants = in.all_participants();
    out._trackPermissions = rconv<TrackPermission, livekit::TrackPermission>(in.track_permissions());
    return out;
}

livekit::SubscriptionPermission ProtoMarshaller::map(SubscriptionPermission in) const
{
    livekit::SubscriptionPermission out;
    out.set_all_participants(std::move(in._allParticipants));
    rconv(std::move(in._trackPermissions), out.mutable_track_permissions());
    return out;
}

SyncState ProtoMarshaller::map(livekit::SyncState in) const
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

livekit::SyncState ProtoMarshaller::map(SyncState in) const
{
    livekit::SyncState out;
    *out.mutable_answer() = map(std::move(in._answer));
    *out.mutable_subscription() = map(std::move(in._subscription));
    rconv(std::move(in._publishTracks), out.mutable_publish_tracks());
    rconv(std::move(in._dataChannels), out.mutable_data_channels());
    *out.mutable_offer() = map(std::move(in._offer));
    rconv(std::move(in._trackSidsDisabled), out.mutable_track_sids_disabled());
    return out;
}

SimulateScenario ProtoMarshaller::map(livekit::SimulateScenario in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::SimulateScenario, SimulateScenario)
            break;
    }
    return out;
}

livekit::SimulateScenario ProtoMarshaller::map(SimulateScenario in) const
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
        default:
            TYPE_CONVERSION_ERROR(SimulateScenario, livekit::SimulateScenario)
            break;
    }
    return out;
}

UpdateParticipantMetadata ProtoMarshaller::map(livekit::UpdateParticipantMetadata in) const
{
    UpdateParticipantMetadata out;
    out._metadata = in.metadata();
    out._name = in.name();
    out._attributes = mconv(in.attributes());
    out._requestId = in.request_id();
    return out;
}

livekit::UpdateParticipantMetadata ProtoMarshaller::map(UpdateParticipantMetadata in) const
{
    livekit::UpdateParticipantMetadata out;
    out.set_metadata(std::move(in._metadata));
    out.set_name(std::move(in._name));
    mconv(std::move(in._attributes), out.mutable_attributes());
    out.set_request_id(in._requestId);
    return out;
}

Ping ProtoMarshaller::map(livekit::Ping in) const
{
    Ping out;
    out._timestamp = in.timestamp();
    out._rtt = in.rtt();
    return out;
}

livekit::Ping ProtoMarshaller::map(Ping in) const
{
    livekit::Ping out;
    out.set_timestamp(in._timestamp);
    out.set_rtt(in._rtt);
    return out;
}

Pong ProtoMarshaller::map(livekit::Pong in) const
{
    Pong out;
    out._lastPingTimestamp = in.last_ping_timestamp();
    out._timestamp = in.timestamp();
    return out;
}

livekit::Pong ProtoMarshaller::map(Pong in) const
{
    livekit::Pong out;
    out.set_last_ping_timestamp(in._lastPingTimestamp);
    out.set_timestamp(in._timestamp);
    return out;
}

UpdateLocalAudioTrack ProtoMarshaller::map(livekit::UpdateLocalAudioTrack in) const
{
    UpdateLocalAudioTrack out;
    out._trackSid = in.track_sid();
    out._features = rconv<AudioTrackFeature, livekit::AudioTrackFeature>(in.features());
    return out;
}

livekit::UpdateLocalAudioTrack ProtoMarshaller::map(UpdateLocalAudioTrack in) const
{
    livekit::UpdateLocalAudioTrack out;
    out.set_track_sid(std::move(in._trackSid));
    rconv(std::move(in._features), out.mutable_features());
    return out;
}

UpdateLocalVideoTrack ProtoMarshaller::map(livekit::UpdateLocalVideoTrack in) const
{
    UpdateLocalVideoTrack out;
    out._trackSid = in.track_sid();
    out._width = in.width();
    out._height = in.height();
    return out;
}

livekit::UpdateLocalVideoTrack ProtoMarshaller::map(UpdateLocalVideoTrack in) const
{
    livekit::UpdateLocalVideoTrack out;
    out.set_track_sid(std::move(in._trackSid));
    out.set_width(in._width);
    out.set_height(in._height);
    return out;
}

ClientInfo ProtoMarshaller::map(livekit::ClientInfo in) const
{
    ClientInfo out;
    out._sdk = map(in.sdk());
    out._version = in.version();
    out._protocol = in.protocol();
    out._os = in.os();
    out._osVersion = in.os_version();
    out._deviceModel = in.device_model();
    out._browser = in.browser();
    out._browserVersion = in.browser_version();
    out._address = in.address();
    out._network = in.network();
    out._otherSdks = in.other_sdks();
    return out;
}

livekit::ClientInfo ProtoMarshaller::map(ClientInfo in) const
{
    livekit::ClientInfo out;
    out.set_sdk(map(in._sdk));
    out.set_version(std::move(in._version));
    out.set_protocol(in._protocol);
    out.set_os(std::move(in._os));
    out.set_os_version(std::move(in._osVersion));
    out.set_device_model(std::move(in._deviceModel));
    out.set_browser(std::move(in._browser));
    out.set_browser_version(std::move(in._browserVersion));
    out.set_address(std::move(in._address));
    out.set_network(std::move(in._network));
    out.set_other_sdks(std::move(in._otherSdks));
    return out;
}

RoomInfo ProtoMarshaller::map(livekit::Room in) const
{
    RoomInfo out;
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

Codec ProtoMarshaller::map(livekit::Codec in) const
{
    return {in.mime(), in.fmtp_line()};
}

TimedVersion ProtoMarshaller::map(livekit::TimedVersion in) const
{
    return {in.unix_micro(), in.ticks()};
}

livekit::TimedVersion ProtoMarshaller::map(TimedVersion in) const
{
    livekit::TimedVersion out;
    out.set_unix_micro(in._unixMicro);
    out.set_ticks(in._ticks);
    return out;
}

ParticipantInfo ProtoMarshaller::map(livekit::ParticipantInfo in) const
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

ParticipantKind ProtoMarshaller::map(livekit::ParticipantInfo_Kind in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::ParticipantInfo_Kind, ParticipantKind)
            break;
    }
    return ParticipantKind::Standard;
}

ParticipantState ProtoMarshaller::map(livekit::ParticipantInfo_State in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::ParticipantInfo_State, ParticipantState)
            break;
    }
    return ParticipantState::Disconnected;
}

ParticipantPermission ProtoMarshaller::map(livekit::ParticipantPermission in) const
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

DisconnectReason ProtoMarshaller::map(livekit::DisconnectReason in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::DisconnectReason, DisconnectReason)
            break;
    }
    return DisconnectReason::UnknownReason;
}

livekit::DisconnectReason ProtoMarshaller::map(DisconnectReason in) const
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
        default:
            TYPE_CONVERSION_ERROR(DisconnectReason, livekit::DisconnectReason)
            break;
    }
    return livekit::UNKNOWN_REASON;
}

TrackSource ProtoMarshaller::map(livekit::TrackSource in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::TrackSource, TrackSource)
            break;
    }
    return TrackSource::Unknown;
}

livekit::TrackSource ProtoMarshaller::map(TrackSource in) const
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
        default:
            TYPE_CONVERSION_ERROR(TrackSource, livekit::TrackSource)
            break;
    }
    return livekit::UNKNOWN;
}

TrackInfo ProtoMarshaller::map(livekit::TrackInfo in) const
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

livekit::TrackInfo ProtoMarshaller::map(TrackInfo in) const
{
    livekit::TrackInfo out;
    out.set_sid(std::move(in._sid));
    out.set_type(map(in._type));
    out.set_name(std::move(in._name));
    out.set_muted(in._muted);
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_simulcast(in._simulcast);
    out.set_disable_dtx(in._disableDtx);
    out.set_source(map(in._source));
    rconv(std::move(in._layers), out.mutable_layers());
    out.set_mime_type(std::move(in._mimeType));
    out.set_mid(std::move(in._mid));
    rconv(std::move(in._codecs), out.mutable_codecs());
    out.set_disable_red(in._disableRed);
    out.set_encryption(map(in._encryption));
    out.set_stream(std::move(in._stream));
    if (in._version.has_value()) {
        *out.mutable_version() = map(std::move(in._version.value()));
    }
    rconv(std::move(in._audioFeatures), out.mutable_audio_features());
    out.set_backup_codec_policy(map(in._backupCodecPolicy));
    return out;
}

VideoQuality ProtoMarshaller::map(livekit::VideoQuality in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::VideoQuality, VideoQuality)
            break;
    }
    return VideoQuality::Low;
}

livekit::VideoQuality ProtoMarshaller::map(VideoQuality in) const
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
        default:
            TYPE_CONVERSION_ERROR(VideoQuality, livekit::VideoQuality)
            break;
    }
    return livekit::LOW;
}

VideoLayer ProtoMarshaller::map(livekit::VideoLayer in) const
{
    VideoLayer out;
    out._quality = map(in.quality());
    out._width = in.width();
    out._height = in.height();
    out._bitrate = in.bitrate();
    out._ssrc = in.ssrc();
    return out;
}

livekit::VideoLayer ProtoMarshaller::map(VideoLayer in) const
{
    livekit::VideoLayer out;
    out.set_quality(map(in._quality));
    out.set_width(in._width);
    out.set_height(in._height);
    out.set_bitrate(in._bitrate);
    out.set_ssrc(in._ssrc);
    return out;
}

TrackType ProtoMarshaller::map(livekit::TrackType in) const
{
    switch (in) {
        case livekit::AUDIO:
            break;
        case livekit::VIDEO:
            return TrackType::Video;
        case livekit::DATA:
            return TrackType::Data;
        default:
            TYPE_CONVERSION_ERROR(livekit::TrackType, TrackType)
            break;
    }
    return TrackType::Audio;
}

livekit::TrackType ProtoMarshaller::map(TrackType in) const
{
    switch (in) {
        case TrackType::Audio:
            break;
        case TrackType::Video:
            return livekit::VIDEO;
        case TrackType::Data:
            return livekit::DATA;
        default:
            TYPE_CONVERSION_ERROR(TrackType, livekit::TrackType)
            break;
    }
    return livekit::AUDIO;
}

SimulcastCodecInfo ProtoMarshaller::map(livekit::SimulcastCodecInfo in) const
{
    SimulcastCodecInfo out;
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    out._cid = in.cid();
    out._layers = rconv<VideoLayer, livekit::VideoLayer>(in.layers());
    return out;
}

livekit::SimulcastCodecInfo ProtoMarshaller::map(SimulcastCodecInfo in) const
{
    livekit::SimulcastCodecInfo out;
    out.set_mime_type(std::move(in._mimeType));
    out.set_mid(std::move(in._mid));
    out.set_cid(std::move(in._cid));
    rconv(std::move(in._layers), out.mutable_layers());
    return out;
}

BackupCodecPolicy ProtoMarshaller::map(livekit::BackupCodecPolicy in) const
{
    switch (in) {
        case livekit::PREFER_REGRESSION:
            break;
        case livekit::REGRESSION:
            return BackupCodecPolicy::Regression;
        case livekit::SIMULCAST:
            return BackupCodecPolicy::Simulcast;
        default:
            TYPE_CONVERSION_ERROR(livekit::BackupCodecPolicy, BackupCodecPolicy)
            break;
    }
    return BackupCodecPolicy::PrefererRegression;
}

livekit::BackupCodecPolicy ProtoMarshaller::map(BackupCodecPolicy in) const
{
    switch (in) {
        case BackupCodecPolicy::PrefererRegression:
            break;
        case BackupCodecPolicy::Regression:
            return livekit::REGRESSION;
        case BackupCodecPolicy::Simulcast:
            return livekit::SIMULCAST;
        default:
            TYPE_CONVERSION_ERROR(BackupCodecPolicy, livekit::BackupCodecPolicy)
            break;
    }
    return livekit::PREFER_REGRESSION;
}

EncryptionType ProtoMarshaller::map(livekit::Encryption_Type in) const
{
    switch (in) {
        case livekit::Encryption_Type_NONE:
            break;
        case livekit::Encryption_Type_GCM:
            return EncryptionType::Gcm;
        case livekit::Encryption_Type_CUSTOM:
            return EncryptionType::Custom;
        default:
            TYPE_CONVERSION_ERROR(livekit::Encryption_Type, EncryptionType)
            break;
    }
    return EncryptionType::None;
}

livekit::Encryption_Type ProtoMarshaller::map(EncryptionType in) const
{
    switch (in) {
        case EncryptionType::None:
            break;
        case EncryptionType::Gcm:
            return livekit::Encryption_Type_GCM;
        case EncryptionType::Custom:
            return livekit::Encryption_Type_CUSTOM;
        default:
            TYPE_CONVERSION_ERROR(EncryptionType, livekit::Encryption_Type)
            break;
    }
    return livekit::Encryption_Type_NONE;
}

AudioTrackFeature ProtoMarshaller::map(livekit::AudioTrackFeature in) const
{
    switch (in) {
        case livekit::TF_STEREO:
            break;
        case livekit::TF_NO_DTX:
            return AudioTrackFeature::NoDtx;
        case livekit::TF_AUTO_GAIN_CONTROL:
            return AudioTrackFeature::AutoGainControl;
        case livekit::TF_ECHO_CANCELLATION:
            return AudioTrackFeature::Echocancellation;
        case livekit::TF_NOISE_SUPPRESSION:
            return AudioTrackFeature::NoiseSuppression;
        case livekit::TF_ENHANCED_NOISE_CANCELLATION:
            return AudioTrackFeature::EnhancedNoiseCancellation;
        default:
            TYPE_CONVERSION_ERROR(livekit::AudioTrackFeature, AudioTrackFeature)
            break;
    }
    return AudioTrackFeature::Stereo;
}

livekit::AudioTrackFeature ProtoMarshaller::map(AudioTrackFeature in) const
{
    switch (in) {
        case AudioTrackFeature::Stereo:
            break;
        case AudioTrackFeature::NoDtx:
            return livekit::TF_NO_DTX;
        case AudioTrackFeature::AutoGainControl:
            return livekit::TF_AUTO_GAIN_CONTROL;
        case AudioTrackFeature::Echocancellation:
            return livekit::TF_ECHO_CANCELLATION;
        case AudioTrackFeature::NoiseSuppression:
            return livekit::TF_NOISE_SUPPRESSION;
        case AudioTrackFeature::EnhancedNoiseCancellation:
            return livekit::TF_ENHANCED_NOISE_CANCELLATION;
        default:
            TYPE_CONVERSION_ERROR(AudioTrackFeature, livekit::AudioTrackFeature)
            break;
    }
    return livekit::TF_STEREO;
}

ClientConfigSetting ProtoMarshaller::map(livekit::ClientConfigSetting in) const
{
    switch (in) {
        case livekit::UNSET:
            break;
        case livekit::DISABLED:
            return ClientConfigSetting::Disabled;
        case livekit::ENABLED:
            return ClientConfigSetting::Enabled;
        default:
            TYPE_CONVERSION_ERROR(livekit::ClientConfigSetting, ClientConfigSetting)
            break;
    }
    return ClientConfigSetting::Unset;
}

ClientConfiguration ProtoMarshaller::map(livekit::ClientConfiguration in) const
{
    ClientConfiguration out;
    out._video = map(in.video());
    out._screen = map(in.screen());
    out._resumeConnection = map(in.resume_connection());
    out._disabledCodecs = map(in.disabled_codecs());
    out._forceRelay = map(in.force_relay());
    return out;
}

DisabledCodecs ProtoMarshaller::map(livekit::DisabledCodecs in) const
{
    DisabledCodecs out;
    out._codecs = rconv<Codec, livekit::Codec>(in.codecs());
    out._publish = rconv<Codec, livekit::Codec>(in.publish());
    return out;
}

VideoConfiguration ProtoMarshaller::map(livekit::VideoConfiguration in) const
{
    VideoConfiguration out;
    out._hardwareEncoder = map(in.hardware_encoder());
    return out;
}

ServerEdition ProtoMarshaller::map(livekit::ServerInfo_Edition in) const
{
    switch (in) {
        case livekit::ServerInfo_Edition_Standard:
            break;
        case livekit::ServerInfo_Edition_Cloud:
            return ServerEdition::Cloud;
        default:
            TYPE_CONVERSION_ERROR(livekit::ServerInfo_Edition, ServerEdition)
            break;
    }
    return ServerEdition::Standard;
}

ServerInfo ProtoMarshaller::map(livekit::ServerInfo in) const
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

SignalTarget ProtoMarshaller::map(livekit::SignalTarget in) const
{
    switch (in) {
        case livekit::PUBLISHER:
            break;
        case livekit::SUBSCRIBER:
            return SignalTarget::Subscriber;
        default:
            TYPE_CONVERSION_ERROR(livekit::SignalTarget, SignalTarget)
            break;
    }
    return SignalTarget::Publisher;
}

livekit::SignalTarget ProtoMarshaller::map(SignalTarget in) const
{
    switch (in) {
        case SignalTarget::Publisher:
            break;
        case SignalTarget::Subscriber:
            return livekit::SUBSCRIBER;
        default:
            TYPE_CONVERSION_ERROR(SignalTarget, livekit::SignalTarget)
            break;
    }
    return livekit::PUBLISHER;
}

LeaveRequestAction ProtoMarshaller::map(livekit::LeaveRequest_Action in) const
{
    switch (in) {
        case livekit::LeaveRequest_Action_DISCONNECT:
            break;
        case livekit::LeaveRequest_Action_RESUME:
            return LeaveRequestAction::Resume;
        case livekit::LeaveRequest_Action_RECONNECT:
            return LeaveRequestAction::Reconnect;
        default:
            TYPE_CONVERSION_ERROR(livekit::LeaveRequest_Action, LeaveRequestAction)
            break;
    }
    return LeaveRequestAction::Disconnect;
}

livekit::LeaveRequest_Action ProtoMarshaller::map(LeaveRequestAction in) const
{
    switch (in) {
        case LeaveRequestAction::Disconnect:
            break;
        case LeaveRequestAction::Resume:
            return livekit::LeaveRequest_Action_RESUME;
        case LeaveRequestAction::Reconnect:
            return livekit::LeaveRequest_Action_RECONNECT;
        default:
            TYPE_CONVERSION_ERROR(LeaveRequestAction, livekit::LeaveRequest_Action)
            break;
    }
    return livekit::LeaveRequest_Action_DISCONNECT;
}

RegionInfo ProtoMarshaller::map(livekit::RegionInfo in) const
{
    RegionInfo out;
    out._region = in.region();
    out._url = in.url();
    out._distance = in.distance();
    return out;
}

livekit::RegionInfo ProtoMarshaller::map(RegionInfo in) const
{
    livekit::RegionInfo out;
    out.set_region(std::move(in._region));
    out.set_url(std::move(in._url));
    out.set_distance(in._distance);
    return out;
}

RegionSettings ProtoMarshaller::map(livekit::RegionSettings in) const
{
    RegionSettings out;
    out._regions = rconv<RegionInfo, livekit::RegionInfo>(in.regions());
    return out;
}

livekit::RegionSettings ProtoMarshaller::map(RegionSettings in) const
{
    livekit::RegionSettings out;
    rconv(std::move(in._regions), out.mutable_regions());
    return out;
}

SpeakerInfo ProtoMarshaller::map(livekit::SpeakerInfo in) const
{
    SpeakerInfo out;
    out._sid = in.sid();
    out._level = in.level();
    out._active = in.active();
    return out;
}

ConnectionQuality ProtoMarshaller::map(livekit::ConnectionQuality in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::ConnectionQuality, ConnectionQuality)
            break;
    }
    return ConnectionQuality::Poor;
}

ConnectionQualityInfo ProtoMarshaller::map(livekit::ConnectionQualityInfo in) const
{
    ConnectionQualityInfo out;
    out._participantSid = in.participant_sid();
    out._quality = map(in.quality());
    out._score = in.score();
    return out;
}

StreamState ProtoMarshaller::map(livekit::StreamState in) const
{
    switch (in) {
        case livekit::ACTIVE:
            break;
        case livekit::PAUSED:
            return StreamState::Paused;
        default:
            TYPE_CONVERSION_ERROR(livekit::StreamState, StreamState)
            break;
    }
    return StreamState::Active;
}

StreamStateInfo ProtoMarshaller::map(livekit::StreamStateInfo in) const
{
    StreamStateInfo out;
    out._participantSid = in.participant_sid();
    out._trackSid = in.track_sid();
    out._state = map(in.state());
    return out;
}

SubscribedQuality ProtoMarshaller::map(livekit::SubscribedQuality in) const
{
    SubscribedQuality out;
    out._quality = map(in.quality());
    out._enabled = in.enabled();
    return out;
}

SubscribedCodec ProtoMarshaller::map(livekit::SubscribedCodec in) const
{
    SubscribedCodec out;
    out._codec = in.codec();
    out._qualities = rconv<SubscribedQuality, livekit::SubscribedQuality>(in.qualities());
    return out;
}

ICEServer ProtoMarshaller::map(livekit::ICEServer in) const
{
    ICEServer out;
    out._urls = rconv<std::string>(in.urls());
    out._username = in.username();
    out._credential = in.credential();
    return out;
}

RequestResponseReason ProtoMarshaller::map(livekit::RequestResponse_Reason in) const
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
        default:
            TYPE_CONVERSION_ERROR(livekit::RequestResponse_Reason, RequestResponseReason)
            break;
    }
    return RequestResponseReason::Ok;
}

SubscriptionError ProtoMarshaller::map(livekit::SubscriptionError in) const
{
    switch (in) {
        case livekit::SE_UNKNOWN:
            break;
        case livekit::SE_CODEC_UNSUPPORTED:
            return SubscriptionError::CodecUnsupported;
        case livekit::SE_TRACK_NOTFOUND:
            return SubscriptionError::TrackNotfound;
        default:
            TYPE_CONVERSION_ERROR(livekit::SubscriptionError, SubscriptionError)
            break;
    }
    return SubscriptionError::Unknown;
}

SimulcastCodec ProtoMarshaller::map(livekit::SimulcastCodec in) const
{
    SimulcastCodec out;
    out._cid = in.cid();
    out._codec = in.codec();
    return out;
}

livekit::SimulcastCodec ProtoMarshaller::map(SimulcastCodec in) const
{
    livekit::SimulcastCodec out;
    out.set_cid(std::move(in._cid));
    out.set_codec(std::move(in._codec));
    return out;
}

ParticipantTracks ProtoMarshaller::map(livekit::ParticipantTracks in) const
{
    ParticipantTracks out;
    out._participantSid = in.participant_sid();
    out._trackSids = rconv<std::string>(in.track_sids());
    return out;
}

livekit::ParticipantTracks ProtoMarshaller::map(ParticipantTracks in) const
{
    livekit::ParticipantTracks out;
    out.set_participant_sid(std::move(in._participantSid));
    rconv(std::move(in._trackSids), out.mutable_track_sids());
    return out;
}

TrackPermission ProtoMarshaller::map(livekit::TrackPermission in) const
{
    TrackPermission out;
    out._participantSid = in.participant_sid();
    out._allAracks = in.all_tracks();
    out._trackSids = rconv<std::string>(in.track_sids());
    out._participantIdentity = in.participant_identity();
    return out;
}

livekit::TrackPermission ProtoMarshaller::map(TrackPermission in) const
{
    livekit::TrackPermission out;
    out.set_participant_sid(std::move(in._participantSid));
    out.set_all_tracks(in._allAracks);
    rconv(std::move(in._trackSids), out.mutable_track_sids());
    out.set_participant_identity(std::move(in._participantIdentity));
    return out;
}

DataChannelInfo ProtoMarshaller::map(livekit::DataChannelInfo in) const
{
    DataChannelInfo out;
    out._label = in.label();
    out._id = in.id();
    out._target = map(in.target());
    return out;
}

livekit::DataChannelInfo ProtoMarshaller::map(DataChannelInfo in) const
{
    livekit::DataChannelInfo out;
    out.set_label(std::move(in._label));
    out.set_id(in._id);
    out.set_target(map(in._target));
    return out;
}

CandidateProtocol ProtoMarshaller::map(livekit::CandidateProtocol in) const
{
    switch (in) {
        case livekit::UDP:
            break;
        case livekit::TCP:
            return CandidateProtocol::Tcp;
        case livekit::TLS:
            return CandidateProtocol::Tls;
        default:
            TYPE_CONVERSION_ERROR(livekit::CandidateProtocol, CandidateProtocol)
            break;
    }
    return CandidateProtocol::Udp;
}

livekit::CandidateProtocol ProtoMarshaller::map(CandidateProtocol in) const
{
    switch (in) {
        case CandidateProtocol::Udp:
            break;
        case CandidateProtocol::Tcp:
            return livekit::TCP;
        case CandidateProtocol::Tls:
            return livekit::TLS;
        default:
            TYPE_CONVERSION_ERROR(CandidateProtocol, livekit::CandidateProtocol)
            break;
    }
    return livekit::UDP;
}

livekit::ClientInfo_SDK ProtoMarshaller::map(SDK sdk) const
{
    switch (sdk) {
        case SDK::Unknown:
            break;
        case SDK::JS:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_JS;
        case SDK::Swift:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_SWIFT;
        case SDK::Android:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_ANDROID;
        case SDK::Flutter:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_FLUTTER;
        case SDK::GO:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_GO;
        case SDK::Unity:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_UNITY;
        case SDK::ReactNative:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_REACT_NATIVE;
        case SDK::Rust:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_RUST;
        case SDK::Python:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_PYTHON;
        case SDK::CPP:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_CPP;
        case SDK::UnityWeb:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_UNITY_WEB;
        case SDK::Node:
            return livekit::ClientInfo_SDK::ClientInfo_SDK_NODE;
        default:
            TYPE_CONVERSION_ERROR(SDK, livekit::ClientInfo_SDK)
            break;
    }
    return livekit::ClientInfo_SDK::ClientInfo_SDK_UNKNOWN;
}

SDK ProtoMarshaller::map(livekit::ClientInfo_SDK sdk) const
{
    switch (sdk) {
        case livekit::ClientInfo_SDK_UNKNOWN:
            break;
        case livekit::ClientInfo_SDK_JS:
            return SDK::JS;
        case livekit::ClientInfo_SDK_SWIFT:
            return SDK::Swift;
        case livekit::ClientInfo_SDK_ANDROID:
            return SDK::Android;
        case livekit::ClientInfo_SDK_FLUTTER:
            return SDK::Flutter;
        case livekit::ClientInfo_SDK_GO:
            return SDK::GO;
        case livekit::ClientInfo_SDK_UNITY:
            return SDK::Unity;
        case livekit::ClientInfo_SDK_REACT_NATIVE:
            return SDK::ReactNative;
        case livekit::ClientInfo_SDK_RUST:
            return SDK::Rust;
        case livekit::ClientInfo_SDK_PYTHON:
            return SDK::Python;
        case livekit::ClientInfo_SDK_CPP:
            return SDK::CPP;
        case livekit::ClientInfo_SDK_UNITY_WEB:
            return SDK::UnityWeb;
        case livekit::ClientInfo_SDK_NODE:
            return SDK::Node;
        default:
            TYPE_CONVERSION_ERROR(livekit::ClientInfo_SDK, SDK)
            break;
    }
    return SDK::Unknown;
}

livekit::DataPacket ProtoMarshaller::map(DataPacket in) const
{
    livekit::DataPacket out;
    out.set_kind(map(in._kind));
    out.set_participant_identity(std::move(in._participantIdentity));
    rconv(std::move(in._destinationIdentities), out.mutable_destination_identities());
    if (std::holds_alternative<UserPacket>(in._value)) {
        auto value = map(std::move(std::get<UserPacket>(in._value)));
        out.set_allocated_user(new livekit::UserPacket(std::move(value)));
    }
    else if (std::holds_alternative<ChatMessage>(in._value)) {
        auto value = map(std::move(std::get<ChatMessage>(in._value)));
        out.set_allocated_chat_message(new livekit::ChatMessage(std::move(value)));
    }
    else if (std::holds_alternative<DataStreamHeader>(in._value)) {
        auto value = map(std::move(std::get<DataStreamHeader>(in._value)));
        out.set_allocated_stream_header(new livekit::DataStream::Header(std::move(value)));
    }
    else if (std::holds_alternative<DataStreamChunk>(in._value)) {
        auto value = map(std::move(std::get<DataStreamChunk>(in._value)));
        out.set_allocated_stream_chunk(new livekit::DataStream::Chunk(std::move(value)));
    }
    else if (std::holds_alternative<DataStreamTrailer>(in._value)) {
        auto value = map(std::move(std::get<DataStreamTrailer>(in._value)));
        out.set_allocated_stream_trailer(new livekit::DataStream::Trailer(std::move(value)));
    }
    return out;
}

DataPacket ProtoMarshaller::map(livekit::DataPacket in) const
{
    DataPacket out;
    out._kind = map(in.kind());
    out._participantIdentity = in.participant_identity();
    out._destinationIdentities = rconv<std::string>(in.destination_identities());
    switch (in.value_case()) {
        case livekit::DataPacket::kUser:
            out._value = map(in.user());
            break;
        case livekit::DataPacket::kChatMessage:
            out._value = map(in.chat_message());
            break;
        case livekit::DataPacket::kStreamHeader:
            out._value = map(in.stream_header());
            break;
        case livekit::DataPacket::kStreamChunk:
            out._value = map(in.stream_chunk());
            break;
        case livekit::DataPacket::kStreamTrailer:
            out._value = map(in.stream_trailer());
            break;
        default:
            logWarning("unsupported data packet value: " + std::to_string(in.value_case()));
            break;
    }
    return out;
}

livekit::DataPacket::Kind ProtoMarshaller::map(DataPacketKind kind) const
{
    switch (kind) {
        case DataPacketKind::Reliable:
            return livekit::DataPacket::Kind::DataPacket_Kind_RELIABLE;
            break;
        case DataPacketKind::Lossy:
            break;
        default:
            TYPE_CONVERSION_ERROR(DataPacketKind, livekit::DataPacket::Kind)
            break;
    }
    return livekit::DataPacket::Kind::DataPacket_Kind_LOSSY;
}

DataPacketKind ProtoMarshaller::map(livekit::DataPacket::Kind kind) const
{
    switch (kind) {
        case livekit::DataPacket_Kind_RELIABLE:
            return DataPacketKind::Reliable;
            break;
        case livekit::DataPacket_Kind_LOSSY:
            break;
        default:
            TYPE_CONVERSION_ERROR(livekit::DataPacket::Kind, DataPacketKind)
            break;
    }
    return DataPacketKind::Lossy;
}

livekit::UserPacket ProtoMarshaller::map(UserPacket in) const
{
    livekit::UserPacket out;
    out.set_participant_sid(std::move(in._participantSid));
    out.set_participant_identity(std::move(in._participantIdentity));
    out.set_payload(std::move(in._payload));
    rconv(std::move(in._destinationSids), out.mutable_destination_sids());
    rconv(std::move(in._destinationIdentities), out.mutable_destination_identities());
    if (in._topic.has_value()) {
        out.set_topic(std::move(in._topic.value()));
    }
    if (in._id.has_value()) {
        out.set_id(std::move(in._id.value()));
    }
    if (in._startTime.has_value()) {
        out.set_start_time(in._startTime.value());
    }
    if (in._endTime.has_value()) {
        out.set_end_time(in._endTime.value());
    }
    out.set_nonce(std::move(in._nonce));
    return out;
}

UserPacket ProtoMarshaller::map(livekit::UserPacket in) const
{
    UserPacket out;
    out._participantSid = in.participant_sid();
    out._participantIdentity = in.participant_identity();
    out._payload = in.payload();
    out._destinationSids = rconv<std::string>(in.destination_sids());
    out._destinationIdentities = rconv<std::string>(in.destination_identities());
    if (in.has_topic()) {
        out._topic = in.topic();
    }
    if (in.has_id()) {
        out._id = in.id();
    }
    if (in.has_start_time()) {
        out._startTime = in.start_time();
    }
    if (in.has_end_time()) {
        out._endTime = in.end_time();
    }
    out._nonce = in.nonce();
    return out;
}

livekit::ChatMessage ProtoMarshaller::map(ChatMessage in) const
{
    livekit::ChatMessage out;
    out.set_id(std::move(in._id));
    out.set_timestamp(in._timestamp);
    if (in._editTimestamp.has_value()) {
        out.set_edit_timestamp(in._editTimestamp.value());
    }
    out.set_deleted(in._deleted);
    out.set_generated(in._generated);
    return out;
}

ChatMessage ProtoMarshaller::map(livekit::ChatMessage in) const
{
    ChatMessage out;
    out._id = in.id();
    out._timestamp = in.timestamp();
    if (in.has_edit_timestamp()) {
        out._editTimestamp = in.edit_timestamp();
    }
    out._deleted = in.deleted();
    out._generated = in.generated();
    return out;
}

livekit::DataStream::OperationType ProtoMarshaller::map(DataStreamTextHeaderOperationType type) const
{
    switch (type) {
        case DataStreamTextHeaderOperationType::Create:
            return livekit::DataStream_OperationType_CREATE;
        case DataStreamTextHeaderOperationType::Update:
            return livekit::DataStream_OperationType_UPDATE;
        case DataStreamTextHeaderOperationType::Delete:
            return livekit::DataStream_OperationType_DELETE;
        case DataStreamTextHeaderOperationType::Reaction:
            break;
        default:
            TYPE_CONVERSION_ERROR(DataStreamTextHeaderOperationType, livekit::DataStream::OperationType)
            break;
    }
    return livekit::DataStream_OperationType_REACTION;
}

DataStreamTextHeaderOperationType ProtoMarshaller::map(livekit::DataStream::OperationType type) const
{
    switch (type) {
        case livekit::DataStream_OperationType_CREATE:
            return DataStreamTextHeaderOperationType::Create;
        case livekit::DataStream_OperationType_UPDATE:
            return DataStreamTextHeaderOperationType::Update;
        case livekit::DataStream_OperationType_DELETE:
            return DataStreamTextHeaderOperationType::Delete;
        case livekit::DataStream_OperationType_REACTION:
            break;
        default:
            TYPE_CONVERSION_ERROR(livekit::DataStream::OperationType, DataStreamTextHeaderOperationType)
            break;
    }
    return DataStreamTextHeaderOperationType::Reaction;
}

livekit::DataStream::ByteHeader ProtoMarshaller::map(DataStreamByteHeader in) const
{
    livekit::DataStream::ByteHeader out;
    out.set_name(std::move(in._name));
    return out;
}

DataStreamByteHeader ProtoMarshaller::map(livekit::DataStream::ByteHeader in) const
{
    DataStreamByteHeader out;
    out._name = in.name();
    return out;
}

livekit::DataStream::Chunk ProtoMarshaller::map(DataStreamChunk in) const
{
    livekit::DataStream::Chunk out;
    out.set_stream_id(std::move(in._streamId));
    out.set_chunk_index(in._chunkIndex);
    out.set_content(std::move(in._content));
    out.set_version(in._version);
    if (in._iv.has_value()) {
        out.set_iv(std::move(in._iv.value()));
    }
    return out;
}

DataStreamChunk ProtoMarshaller::map(livekit::DataStream::Chunk in) const
{
    DataStreamChunk out;
    out._streamId = in.stream_id();
    out._chunkIndex = in.chunk_index();
    out._content = in.content();
    out._version = in.version();
    if (in.has_iv()) {
        out._iv = in.iv();
    }
    return out;
}

livekit::DataStream::TextHeader ProtoMarshaller::map(DataStreamTextHeader in) const
{
    livekit::DataStream::TextHeader out;
    out.set_operation_type(map(in._operationType));
    out.set_version(in._version);
    out.set_reply_to_stream_id(std::move(in._replyToStreamId));
    rconv(std::move(in._attachedStreamIds), out.mutable_attached_stream_ids());
    out.set_generated(in._generated);
    return out;
}

DataStreamTextHeader ProtoMarshaller::map(livekit::DataStream::TextHeader in) const
{
    DataStreamTextHeader out;
    out._operationType = map(in.operation_type());
    out._version = in.version();
    out._replyToStreamId = in.reply_to_stream_id();
    out._attachedStreamIds = rconv<std::string>(in.attached_stream_ids());
    out._generated = in.generated();
    return out;
}

livekit::DataStream::Header ProtoMarshaller::map(DataStreamHeader in) const
{
    livekit::DataStream::Header out;
    out.set_stream_id(std::move(in._streamId));
    out.set_timestamp(in._timestamp);
    out.set_topic(std::move(in._topic));
    out.set_mime_type(std::move(in._mimeType));
    if (in._totalLength.has_value()) {
        out.set_total_length(in._totalLength.value());
    }
    out.set_encryption_type(map(in._encryptionType));
    mconv(std::move(in._attributes), out.mutable_attributes());
    if (std::holds_alternative<DataStreamByteHeader>(in._contentHeader)) {
        auto value = map(std::get<DataStreamByteHeader>(std::move(in._contentHeader)));
        out.set_allocated_byte_header(new livekit::DataStream::ByteHeader(std::move(value)));
    }
    else if (std::holds_alternative<DataStreamTextHeader>(in._contentHeader)) {
        auto value = map(std::get<DataStreamTextHeader>(std::move(in._contentHeader)));
        out.set_allocated_text_header(new livekit::DataStream::TextHeader(std::move(value)));
    }
    return out;
}

DataStreamHeader ProtoMarshaller::map(livekit::DataStream::Header in) const
{
    DataStreamHeader out;
    out._streamId = in.stream_id();
    out._timestamp = in.timestamp();
    out._topic = in.topic();
    out._mimeType = in.mime_type();
    if (in.has_total_length()) {
        out._totalLength = in.total_length();
    }
    out._encryptionType = map(in.encryption_type());
    out._attributes = mconv(in.attributes());
    switch (in.content_header_case()) {
        case livekit::DataStream_Header::kTextHeader:
            out._contentHeader = map(in.text_header());
            break;
        case livekit::DataStream_Header::kByteHeader:
            out._contentHeader = map(in.byte_header());
            break;
        default:
            logWarning("unsupported data stream header: " + std::to_string(in.content_header_case()));
            break;
    }
    return out;
}

livekit::DataStream::Trailer ProtoMarshaller::map(DataStreamTrailer in) const
{
    livekit::DataStream::Trailer out;
    out.set_stream_id(std::move(in._streamId));
    out.set_reason(std::move(in._reason));
    mconv(std::move(in._attributes), out.mutable_attributes());
    return out;
}

DataStreamTrailer ProtoMarshaller::map(livekit::DataStream::Trailer in) const
{
    DataStreamTrailer out;
    out._streamId = in.stream_id();
    out._reason = in.reason();
    out._attributes = mconv(in.attributes());
    return out;
}

std::string_view ProtoMarshaller::logCategory() const
{
    static const std::string_view category("proto_marshaller");
    return category;
}

template <typename TOut, typename TIn, class TProtoBufRepeated>
std::vector<TOut> ProtoMarshaller::rconv(TProtoBufRepeated in) const
{
    return fromProtoRepeated<TOut>(std::move(in), [this](auto v) { return map(TIn(std::move(v))); });
}

template <typename TCppRepeated, class TProtoBufRepeated>
void ProtoMarshaller::rconv(TCppRepeated from, TProtoBufRepeated* to) const
{
    if (to) {
        toProtoRepeated(std::move(from), to, [this](auto v) { return map(std::move(v)); });
    }
}

template <typename K, typename V>
std::unordered_map<K, V> ProtoMarshaller::mconv(google::protobuf::Map<K, V> in) const
{
    return fromProtoMap(std::move(in));
}

template <typename K, typename V>
void ProtoMarshaller::mconv(std::unordered_map<K, V> from,
                            google::protobuf::Map<K, V>* to) const
{
    toProtoMap(std::move(from), to);
}

IceCandidate::IceCandidate(std::string sdp, std::string sdpMid,
                           int sdpMLineIndex,
                           std::string usernameFragment)
    : _sdp(std::move(sdp))
    , _sdpMid(std::move(sdpMid))
    , _sdpMLineIndex(sdpMLineIndex)
    , _usernameFragment(std::move(usernameFragment))
{
}

} // namespace LiveKitCpp
