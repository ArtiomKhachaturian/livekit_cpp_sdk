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
#include "ResponseInterceptor.h"
#include "MarshalledTypesFwd.h"
#include "ProtoUtils.h"
#include "livekit/signaling/SignalServerListener.h"

namespace {

inline std::string formatVerboseMsg(const std::string& typeName) {
    if (!typeName.empty()) {
        return  "signal '" + typeName + "' received from server";
    }
    return {};
}

template <typename T>
inline std::string formatVerboseMsg() {
    return formatVerboseMsg(LiveKitCpp::marshalledTypeName<T>());
}

}

namespace LiveKitCpp
{

MARSHALLED_TYPE_NAME_DECL(livekit::JoinResponse)
MARSHALLED_TYPE_NAME_DECL(livekit::SessionDescription)
MARSHALLED_TYPE_NAME_DECL(livekit::TrickleRequest)
MARSHALLED_TYPE_NAME_DECL(livekit::ParticipantUpdate)
MARSHALLED_TYPE_NAME_DECL(livekit::TrackPublishedResponse)
MARSHALLED_TYPE_NAME_DECL(livekit::LeaveRequest)
MARSHALLED_TYPE_NAME_DECL(livekit::MuteTrackRequest)
MARSHALLED_TYPE_NAME_DECL(livekit::SpeakersChanged)
MARSHALLED_TYPE_NAME_DECL(livekit::RoomUpdate)
MARSHALLED_TYPE_NAME_DECL(livekit::ConnectionQualityUpdate)
MARSHALLED_TYPE_NAME_DECL(livekit::StreamStateUpdate)
MARSHALLED_TYPE_NAME_DECL(livekit::SubscribedQualityUpdate)
MARSHALLED_TYPE_NAME_DECL(livekit::SubscriptionPermissionUpdate)
MARSHALLED_TYPE_NAME_DECL(livekit::TrackUnpublishedResponse)
MARSHALLED_TYPE_NAME_DECL(livekit::ReconnectResponse)
MARSHALLED_TYPE_NAME_DECL(livekit::SubscriptionResponse)
MARSHALLED_TYPE_NAME_DECL(livekit::RequestResponse)
MARSHALLED_TYPE_NAME_DECL(livekit::TrackSubscribed)
MARSHALLED_TYPE_NAME_DECL(livekit::Pong)

ResponseInterceptor::ResponseInterceptor(Bricks::Logger* logger)
    : Bricks::LoggableR<>(logger)
    , _marshaller(logger)
{
}

void ResponseInterceptor::parseBinary(const void* data, size_t dataLen)
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
                        logVerbose(formatVerboseMsg("RefreshToken"));
                        notify(&SignalServerListener::onRefreshToken, response->refresh_token());
                    }
                    break;
                case livekit::SignalResponse::kTrackUnpublished:
                    handle(response->track_unpublished());
                    break;
                case livekit::SignalResponse::kPong: // deprecated
                    if (response->has_pong()) {
                        logVerbose(formatVerboseMsg<livekit::Pong>());
                        Pong pong;
                        pong._timestamp = response->pong();
                        notify(&SignalServerListener::onPong, pong);
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

std::string_view ResponseInterceptor::logCategory() const
{
    static const std::string_view category("response_interceptor");
    return category;
}

std::optional<livekit::SignalResponse> ResponseInterceptor::
    parse(const void* data, size_t dataLen) const
{
    return protoFromBytes<livekit::SignalResponse>(data, dataLen, logger(), logCategory());
}

template <class Method, typename... Args>
void ResponseInterceptor::notify(const Method& method, Args&&... args) const
{
    _listener.invoke(method, std::forward<Args>(args)...);
}

template <class Method, class TLiveKitType>
void ResponseInterceptor::signal(const Method& method, const TLiveKitType& sig,
                                 std::string typeName) const
{
    if (typeName.empty()) {
        typeName = marshalledTypeName<TLiveKitType>();
    }
    logVerbose(formatVerboseMsg(typeName));
    notify(method, _marshaller.map(sig));
}

void ResponseInterceptor::handle(const livekit::JoinResponse& response) const
{
    signal(&SignalServerListener::onJoin, response);
}

void ResponseInterceptor::handle(const livekit::SessionDescription& desc, bool offer) const
{
    auto typeName = marshalledTypeName<livekit::SessionDescription>();
    if (offer) {
        signal(&SignalServerListener::onOffer, desc, typeName + "/offer");
    }
    else {
        signal(&SignalServerListener::onAnswer, desc, typeName + "/answer");
    }
}

void ResponseInterceptor::handle(const livekit::TrickleRequest& request) const
{
    signal(&SignalServerListener::onTrickle, request);
}

void ResponseInterceptor::handle(const livekit::ParticipantUpdate& update) const
{
    signal(&SignalServerListener::onUpdate, update);
}

void ResponseInterceptor::handle(const livekit::TrackPublishedResponse& response) const
{
    signal(&SignalServerListener::onTrackPublished, response);
}

void ResponseInterceptor::handle(const livekit::LeaveRequest& request) const
{
    signal(&SignalServerListener::onLeave, request);
}

void ResponseInterceptor::handle(const livekit::MuteTrackRequest& request) const
{
    signal(&SignalServerListener::onMute, request);
}

void ResponseInterceptor::handle(const livekit::SpeakersChanged& changed) const
{
    signal(&SignalServerListener::onSpeakersChanged, changed);
}

void ResponseInterceptor::handle(const livekit::RoomUpdate& update) const
{
    signal(&SignalServerListener::onRoomUpdate, update);
}

void ResponseInterceptor::handle(const livekit::ConnectionQualityUpdate& update) const
{
    signal(&SignalServerListener::onConnectionQuality, update);
}

void ResponseInterceptor::handle(const livekit::StreamStateUpdate& update) const
{
    signal(&SignalServerListener::onStreamStateUpdate, update);
}

void ResponseInterceptor::handle(const livekit::SubscribedQualityUpdate& update) const
{
    signal(&SignalServerListener::onSubscribedQualityUpdate, update);
}

void ResponseInterceptor::handle(const livekit::SubscriptionPermissionUpdate& update) const
{
    signal(&SignalServerListener::onSubscriptionPermission, update);
}

void ResponseInterceptor::handle(const livekit::TrackUnpublishedResponse& response) const
{
    signal(&SignalServerListener::onTrackUnpublished, response);
}

void ResponseInterceptor::handle(const livekit::ReconnectResponse& response) const
{
    signal(&SignalServerListener::onReconnect, response);
}

void ResponseInterceptor::handle(const livekit::SubscriptionResponse& response) const
{
    signal(&SignalServerListener::onSubscriptionResponse, response);
}

void ResponseInterceptor::handle(const livekit::RequestResponse& response) const
{
    signal(&SignalServerListener::onRequestResponse, response);
}

void ResponseInterceptor::handle(const livekit::TrackSubscribed& subscribed) const
{
    signal(&SignalServerListener::onTrackSubscribed, subscribed);
}

void ResponseInterceptor::handle(const livekit::Pong& pong) const
{
    signal(&SignalServerListener::onPong, pong);
}

} // namespace LiveKitCpp
