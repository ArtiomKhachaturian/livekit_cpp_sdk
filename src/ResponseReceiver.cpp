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
#include "ResponseReceiver.h"
#include "SignalServerListener.h"

namespace {

template <typename TProto>
inline std::optional<TProto> fromBytes(const void* data, size_t dataLen) {
    if (data && dataLen) {
        TProto instance;
        if (instance.ParseFromArray(data, int(dataLen))) {
            return instance;
        }
    }
    return std::nullopt;
}

}

namespace LiveKitCpp
{

ResponseReceiver::ResponseReceiver(uint64_t signalClientId, Bricks::Logger* logger)
    : _signalClientId(signalClientId)
    , _signals(logger)
{
}

void ResponseReceiver::parseBinary(const void* data, size_t dataLen)
{
    if (_listener) {
        if (const auto response = parse(data, dataLen)) {
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
                    if (response->has_refresh_token()) {
                        notify(&SignalServerListener::onRefreshToken, response->refresh_token());
                    }
                    break;
                case livekit::SignalResponse::kTrackUnpublished:
                    handle(response->track_unpublished());
                    break;
                case livekit::SignalResponse::kPong: // deprecated
                    if (response->has_pong()) {
                        notify(&SignalServerListener::onPong, response->pong(), int64_t{});
                    }
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
            notify(&SignalServerListener::onSignalParseError);
        }
    }
}

void ResponseReceiver::setListener(SignalServerListener* listener)
{
    _listener = listener;
}

std::optional<livekit::SignalResponse> ResponseReceiver::parse(const void* data,
                                                               size_t dataLen)
{
    return fromBytes<livekit::SignalResponse>(data, dataLen);
}

template <class Method, typename... Args>
void ResponseReceiver::notify(const Method& method, Args&&... args) const
{
    _listener.invoke(method, _signalClientId, std::forward<Args>(args)...);
}

template <class Method, class TLiveKitType>
void ResponseReceiver::signal(const Method& method, const TLiveKitType& sig) const
{
    notify(method, _signals.map(sig));
}

void ResponseReceiver::handle(const livekit::JoinResponse& response) const
{
    signal(&SignalServerListener::onJoin, response);
}

void ResponseReceiver::handle(const livekit::SessionDescription& desc, bool offer) const
{
    if (offer) {
        signal(&SignalServerListener::onOffer, desc);
    }
    else {
        signal(&SignalServerListener::onAnswer, desc);
    }
}

void ResponseReceiver::handle(const livekit::TrickleRequest& request) const
{
    signal(&SignalServerListener::onTrickle, request);
}

void ResponseReceiver::handle(const livekit::ParticipantUpdate& update) const
{
    signal(&SignalServerListener::onUpdate, update);
}

void ResponseReceiver::handle(const livekit::TrackPublishedResponse& response) const
{
    signal(&SignalServerListener::onTrackPublished, response);
}

void ResponseReceiver::handle(const livekit::LeaveRequest& request) const
{
    signal(&SignalServerListener::onLeave, request);
}

void ResponseReceiver::handle(const livekit::MuteTrackRequest& request) const
{
    signal(&SignalServerListener::onMute, request);
}

void ResponseReceiver::handle(const livekit::SpeakersChanged& changed) const
{
    signal(&SignalServerListener::onSpeakersChanged, changed);
}

void ResponseReceiver::handle(const livekit::RoomUpdate& update) const
{
    signal(&SignalServerListener::onRoomUpdate, update);
}

void ResponseReceiver::handle(const livekit::ConnectionQualityUpdate& update) const
{
    signal(&SignalServerListener::onConnectionQuality, update);
}

void ResponseReceiver::handle(const livekit::StreamStateUpdate& update) const
{
    signal(&SignalServerListener::onStreamStateUpdate, update);
}

void ResponseReceiver::handle(const livekit::SubscribedQualityUpdate& update) const
{
    signal(&SignalServerListener::onSubscribedQualityUpdate, update);
}

void ResponseReceiver::handle(const livekit::SubscriptionPermissionUpdate& update) const
{
    signal(&SignalServerListener::onSubscriptionPermission, update);
}

void ResponseReceiver::handle(const livekit::TrackUnpublishedResponse& response) const
{
    signal(&SignalServerListener::onTrackUnpublished, response);
}

void ResponseReceiver::handle(const livekit::ReconnectResponse& response) const
{
    signal(&SignalServerListener::onReconnect, response);
}

void ResponseReceiver::handle(const livekit::SubscriptionResponse& response) const
{
    signal(&SignalServerListener::onSubscriptionResponse, response);
}

void ResponseReceiver::handle(const livekit::RequestResponse& response) const
{
    signal(&SignalServerListener::onRequestResponse, response);
}

void ResponseReceiver::handle(const livekit::TrackSubscribed& subscribed) const
{
    signal(&SignalServerListener::onTrackSubscribed, subscribed);
}

void ResponseReceiver::handle(const livekit::Pong& pong) const
{
    notify(&SignalServerListener::onPong, pong.timestamp(), pong.last_ping_timestamp());
}

} // namespace LiveKitCpp
