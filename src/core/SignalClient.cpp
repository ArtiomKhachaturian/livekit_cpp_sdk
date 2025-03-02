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
#include "SignalClient.h"
#include "SignalClientListener.h"
#include "ProtectedObjAliases.h"
#include "Listeners.h"
#include "rtc/JoinResponse.h"
#include "livekit_rtc.pb.h"
#include <optional>

namespace LiveKitCpp
{

class SignalClient::Impl
{
public:
    Impl(uint64_t id, CommandSender* commandSender);
    State transportState() const noexcept;
    bool changeTransportState(State state);
    void parseBinary(const void* data, size_t dataLen);
    void addListener(SignalClientListener* listener);
    void removeListener(SignalClientListener* listener);
private:
    template <class Method, typename... Args>
    void invokeListener(const Method& method, Args&&... args) const;
    void handle(const livekit::JoinResponse& join);
    void handle(const livekit::SessionDescription& desc, bool offer);
    void handle(const livekit::TrickleRequest& trickle);
    void handle(const livekit::ParticipantUpdate& participantUpdate);
    void handle(const livekit::TrackPublishedResponse& trackPublished);
    void handle(const livekit::LeaveRequest& leave);
    void handle(const livekit::MuteTrackRequest& muteTrack);
    void handle(const livekit::SpeakersChanged& speakersChanged);
    void handle(const livekit::RoomUpdate& roomtUpdate);
    void handle(const livekit::ConnectionQualityUpdate& qualityUpdate);
    void handle(const livekit::StreamStateUpdate& stateUpdate);
    void handle(const livekit::SubscribedQualityUpdate& qualityUpdate);
    void handle(const livekit::SubscriptionPermissionUpdate& permissionUpdate);
    void handle(const livekit::TrackUnpublishedResponse& trackUnpublished);
    void handle(const livekit::ReconnectResponse& reconnect);
    void handle(const livekit::SubscriptionResponse& subscription);
    void handle(const livekit::RequestResponse& request);
    void handle(const livekit::TrackSubscribed& trackSubscribed);
    void handle(const livekit::Pong& pong);
    template <typename TProtoBufType>
    static std::optional<TProtoBufType> parse(const void* data, size_t dataLen);
    template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
    static std::vector<TCppType> mapFrom(const TProtoBufRepeated& in);
    static std::optional<livekit::SignalResponse>
        parseSignalResponse(const void* data, size_t dataLen);
    static Room mapFrom(const livekit::Room& in);
    static Codec mapFrom(const livekit::Codec& in);
    static TimedVersion mapFrom(const livekit::TimedVersion& in);
    static ParticipantInfo mapFrom(const livekit::ParticipantInfo& in);
    static ParticipantKind mapFrom(livekit::ParticipantInfo_Kind in);
    static ParticipantState mapFrom(livekit::ParticipantInfo_State in);
    static ParticipantPermission mapFrom(const livekit::ParticipantPermission& in);
    static DisconnectReason mapFrom(livekit::DisconnectReason in);
    static TrackSource mapFrom(livekit::TrackSource in);
    static TrackInfo mapFrom(const livekit::TrackInfo& in);
private:
    const uint64_t _id;
    CommandSender* const _commandSender;
    Listeners<SignalClientListener*> _listeners;
    ProtectedObj<State> _transportState = State::Disconnected;
};

SignalClient::SignalClient(CommandSender* commandSender)
    : _impl(std::make_unique<Impl>(id(), commandSender))
{
}

SignalClient::~SignalClient()
{
}

bool SignalClient::connect()
{
    return changeTransportState(State::Connected);
}

void SignalClient::disconnect()
{
    changeTransportState(State::Disconnected);
}

State SignalClient::transportState() const noexcept
{
    return _impl->transportState();
}

void SignalClient::addListener(SignalClientListener* listener)
{
    _impl->addListener(listener);
}

void SignalClient::removeListener(SignalClientListener* listener)
{
    _impl->removeListener(listener);
}

void SignalClient::receiveBinary(const void* data, size_t dataLen)
{
    _impl->parseBinary(data, dataLen);
}

void SignalClient::receiveText(const std::string_view& /*text*/)
{
}

bool SignalClient::changeTransportState(State state)
{
    return _impl->changeTransportState(state);
}

SignalClient::Impl::Impl(uint64_t id, CommandSender* commandSender)
    : _id(id)
    , _commandSender(commandSender)
{
}

State SignalClient::Impl::transportState() const noexcept
{
    LOCK_READ_PROTECTED_OBJ(_transportState);
    return _transportState.constRef();
}

bool SignalClient::Impl::changeTransportState(State state)
{
    bool accepted = false, changed = false;
    {
        LOCK_WRITE_PROTECTED_OBJ(_transportState);
        if (_transportState != state) {
            switch(_transportState.constRef()) {
                case State::Connecting:
                    // any state is good
                    accepted = true;
                    break;
                case State::Connected:
                    accepted = State::Disconnecting == state || State::Disconnected == state;
                    break;
                case State::Disconnecting:
                    accepted = State::Disconnected == state;
                    break;
                case State::Disconnected:
                    accepted = State::Connecting == state || State::Connected == state;
                    break;
            }
            if (accepted) {
                _transportState = state;
                changed = true;
            }
        }
        else {
            accepted = true;
        }
    }
    if (changed) {
        invokeListener(&SignalClientListener::onTransportStateChanged, state);
    }
    return accepted;
}

void SignalClient::Impl::parseBinary(const void* data, size_t dataLen)
{
    if (auto response = parseSignalResponse(data, dataLen)) {
        switch (response->message_case()) {
            case livekit::SignalResponse::kJoin:
                handle(response->join());
                break;
            case livekit::SignalResponse::kAnswer:
                handle(response->answer(), false);
                break;
            case livekit::SignalResponse::kOffer:
                handle(response->offer(), true);
                break;
            case livekit::SignalResponse::kTrickle:
                handle(response->trickle());
                break;
            case livekit::SignalResponse::kUpdate:
                handle(response->update());
                break;
            case livekit::SignalResponse::kTrackPublished:
                handle(response->track_published());
                break;
            case livekit::SignalResponse::kLeave:
                handle(response->leave());
                break;
            case livekit::SignalResponse::kMute:
                handle(response->mute());
                break;
            case livekit::SignalResponse::kSpeakersChanged:
                handle(response->speakers_changed());
                break;
            case livekit::SignalResponse::kRoomUpdate:
                handle(response->room_update());
                break;
            case livekit::SignalResponse::kConnectionQuality:
                handle(response->connection_quality());
                break;
            case livekit::SignalResponse::kStreamStateUpdate:
                handle(response->stream_state_update());
                break;
            case livekit::SignalResponse::kSubscribedQualityUpdate:
                handle(response->subscribed_quality_update());
                break;
            case livekit::SignalResponse::kSubscriptionPermissionUpdate:
                handle(response->subscription_permission_update());
                break;
            case livekit::SignalResponse::kRefreshToken:
                invokeListener(&SignalClientListener::onRefreshToken, response->refresh_token());
                break;
            case livekit::SignalResponse::kTrackUnpublished:
                handle(response->track_unpublished());
                break;
            case livekit::SignalResponse::kPong: // deprecated
                invokeListener(&SignalClientListener::onPong,
                               std::chrono::milliseconds{response->pong()},
                               std::chrono::milliseconds{});
                break;
            case livekit::SignalResponse::kReconnect:
                handle(response->reconnect());
                break;
            case livekit::SignalResponse::kPongResp:
                handle(response->pong_resp());
                break;
            case livekit::SignalResponse::kSubscriptionResponse:
                handle(response->subscription_response());
                break;
            case livekit::SignalResponse::kRequestResponse:
                handle(response->request_response());
                break;
            case livekit::SignalResponse::kTrackSubscribed:
                handle(response->track_subscribed());
                break;
            default:
                // TODO: dump response to log
                break;
        }
    }
    else {
        invokeListener(&SignalClientListener::onServerResponseParseError);
    }
}

void SignalClient::Impl::addListener(SignalClientListener* listener)
{
    _listeners.add(listener);
}

void SignalClient::Impl::removeListener(SignalClientListener* listener)
{
    _listeners.remove(listener);
}

template <class Method, typename... Args>
void SignalClient::Impl::invokeListener(const Method& method, Args&&... args) const
{
    _listeners.invokeMethod(method, _id, std::forward<Args>(args)...);
}

void SignalClient::Impl::handle(const livekit::JoinResponse& join)
{
    JoinResponse resp;
    if (join.has_room()) {
        resp._room = mapFrom(join.room());
    }
    if (join.has_participant()) {
        resp._participant = mapFrom(join.participant());
    }
    resp._otherParticipants = mapFrom<ParticipantInfo, livekit::ParticipantInfo>(join.other_participants());
    resp._serverVersion = join.server_version();
    resp._subscriberPrimary = join.subscriber_primary();
    resp._alternativeUrl = join.alternative_url();
    resp._serverRegion = join.server_region();
    resp._pingTimeout = join.ping_timeout();
    resp._pingInterval = join.ping_interval();
    resp._sifTrailer = join.sif_trailer();
    resp._enabledPublishCodecs = mapFrom<Codec, livekit::Codec>(join.enabled_publish_codecs());
    resp._fastPublish = join.fast_publish();
    invokeListener(&SignalClientListener::onJoin, resp);
}

void SignalClient::Impl::handle(const livekit::SessionDescription& desc, bool offer)
{
    if (offer) {
        invokeListener(&SignalClientListener::onOffer, desc.type(), desc.sdp());
    }
    else {
        invokeListener(&SignalClientListener::onAnswer, desc.type(), desc.sdp());
    }
}

void SignalClient::Impl::handle(const livekit::TrickleRequest& trickle)
{
    
}

void SignalClient::Impl::handle(const livekit::ParticipantUpdate& participantUpdate)
{
    
}

void SignalClient::Impl::handle(const livekit::TrackPublishedResponse& trackPublished)
{
    
}

void SignalClient::Impl::handle(const livekit::LeaveRequest& leave)
{
    
}

void SignalClient::Impl::handle(const livekit::MuteTrackRequest& muteTrack)
{
    
}

void SignalClient::Impl::handle(const livekit::SpeakersChanged& speakersChanged)
{
    
}

void SignalClient::Impl::handle(const livekit::RoomUpdate& roomtUpdate)
{
    
}

void SignalClient::Impl::handle(const livekit::ConnectionQualityUpdate& qualityUpdate)
{
    
}

void SignalClient::Impl::handle(const livekit::StreamStateUpdate& stateUpdate)
{
    
}

void SignalClient::Impl::handle(const livekit::SubscribedQualityUpdate& qualityUpdate)
{
    
}

void SignalClient::Impl::handle(const livekit::SubscriptionPermissionUpdate& permissionUpdate)
{
    
}

void SignalClient::Impl::handle(const livekit::TrackUnpublishedResponse& trackUnpublished)
{
    
}

void SignalClient::Impl::handle(const livekit::ReconnectResponse& reconnect)
{
    
}

void SignalClient::Impl::handle(const livekit::SubscriptionResponse& subscription)
{
    
}

void SignalClient::Impl::handle(const livekit::RequestResponse& request)
{
    
}

void SignalClient::Impl::handle(const livekit::TrackSubscribed& trackSubscribed)
{
    
}

void SignalClient::Impl::handle(const livekit::Pong& pong)
{
    invokeListener(&SignalClientListener::onPong,
                   std::chrono::milliseconds{pong.timestamp()},
                   std::chrono::milliseconds{pong.last_ping_timestamp()});
}

template <class TProtoBufType>
std::optional<TProtoBufType> SignalClient::Impl::parse(const void* data,
                                                         size_t dataLen) {
    if (data && dataLen) {
        TProtoBufType instance;
        if (instance.ParseFromArray(data, int(dataLen))) {
            return instance;
        }
    }
    return std::nullopt;
}

template <typename TCppType, typename TProtoBufType, class TProtoBufRepeated>
std::vector<TCppType> SignalClient::Impl::mapFrom(const TProtoBufRepeated& in)
{
    if (const auto size = in.size()) {
        std::vector<TCppType> out;
        out.reserve(size_t(size));
        for (const auto& val : in) {
            out.push_back(mapFrom(TProtoBufType(val)));
        }
        return out;
    }
    return {};
}

std::optional<livekit::SignalResponse> SignalClient::Impl::
    parseSignalResponse(const void* data, size_t dataLen)
{
    return parse<livekit::SignalResponse>(data, dataLen);
}

Room SignalClient::Impl::mapFrom(const livekit::Room& in)
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
    out._enabledCodecs = mapFrom<Codec, livekit::Codec>(in.enabled_codecs());
    out._metadata = in.metadata();
    out._numParticipants = in.num_participants();
    out._numPublishers = in.num_publishers();
    out._activeRecording = in.active_recording();
    if (in.has_version()) {
        out._version = mapFrom(in.version());
    }
    return out;
}

Codec SignalClient::Impl::mapFrom(const livekit::Codec& in)
{
    return {in.mime(), in.fmtp_line()};
}

TimedVersion SignalClient::Impl::mapFrom(const livekit::TimedVersion& in)
{
    return {in.unix_micro(), in.ticks()};
}

ParticipantInfo SignalClient::Impl::mapFrom(const livekit::ParticipantInfo& in)
{
    ParticipantInfo out;
    out._sid = in.sid();
    out._identity = in.identity();
    out._state = mapFrom(in.state());
    out._metadata = in.metadata();
    out._joinedAt = in.joined_at();
    out.joinedAtMs = in.joined_at_ms();
    out._name = in.name();
    out._version = in.version();
    if (in.has_permission()) {
        out._permission = mapFrom(in.permission());
    }
    out._region = in.region();
    out._isPublisher = in.is_publisher();
    out._kind = mapFrom(in.kind());
    //out._attributes
    out._disconnectReason = mapFrom(in.disconnect_reason());
    return out;
}

ParticipantKind SignalClient::Impl::mapFrom(livekit::ParticipantInfo_Kind in)
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

ParticipantState SignalClient::Impl::mapFrom(livekit::ParticipantInfo_State in)
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

ParticipantPermission SignalClient::Impl::mapFrom(const livekit::ParticipantPermission& in)
{
    ParticipantPermission out;
    out._canSubscribe = in.can_subscribe();
    out._canPublish = in.can_publish();
    out._canPublish_data = in.can_publish_data();
    out._canPublishSources = mapFrom<TrackSource, livekit::TrackSource>(in.can_publish_sources());
    out._hidden = in.hidden();
    out._recorder = in.recorder();
    out._canUpdateMetadata = in.can_update_metadata();
    out._agent = in.agent();
    out._canSubscribeMetrics = in.can_subscribe_metrics();
    return out;
}

DisconnectReason SignalClient::Impl::mapFrom(livekit::DisconnectReason in)
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

TrackSource SignalClient::Impl::mapFrom(livekit::TrackSource in)
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

TrackInfo SignalClient::Impl::mapFrom(const livekit::TrackInfo& in)
{
    TrackInfo out;
    out._sid = in.sid();
    //out._type = in.type();
    out._name = in.name();
    out._muted = in.muted();
    out._width = in.width();
    out._height = in.height();
    out._simulcast = in.simulcast();
    out._disableDtx = in.disable_dtx();
    out._source = mapFrom(in.source());
    //out._layers = mapFrom<>(in.layers());
    out._mimeType = in.mime_type();
    out._mid = in.mid();
    //out._codecs = mapFrom<>(in.codecs());
    out._stereo = in.stereo();
    out._disableRed = in.disable_red();
    //out._encryption = mapFrom(in.encryption());
    out._stream = in.stream();
    if (in.has_version()) {
        out._version = mapFrom(in.version());
    }
    //out._audioFeatures = mapFrom<>(in.audio_features());
    //out._backupCodecPolicy = mapFrom(in.backup_codec_policy());
    return out;
}

} // namespace LiveKitCpp
