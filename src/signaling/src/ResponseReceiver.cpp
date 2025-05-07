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
#include "MarshalledTypesFwd.h"
#include "ProtoUtils.h"
#include "livekit/signaling/ResponsesListener.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite.h>

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
MARSHALLED_TYPE_NAME_DECL(livekit::DataPacket)
MARSHALLED_TYPE_NAME_DECL(livekit::RoomMovedResponse)

ResponseReceiver::ResponseReceiver(Bricks::Logger* logger)
    : Bricks::LoggableR<>(logger)
    , _marshaller(logger)
{
}

void ResponseReceiver::parseBinary(const void* data, size_t dataLen)
{
    if (_listener && data && dataLen) {
        if (const auto response = parseResponse(data, dataLen)) {
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
                        notify(&ResponsesListener::onRefreshToken, response->refresh_token());
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
                        notify(&ResponsesListener::onPong, pong);
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
                case livekit::SignalResponse::kRoomMoved:
                    handle(response->room_moved());
                    break;
                default:
                    // TODO: dump response to log
                    break;
            }
        }
        else if (const auto dataPacket = parseDataPacket(data, dataLen)) {
            handle(dataPacket.value());
        }
        else {
            notifyAboutError("unknown proto packet, size is " + std::to_string(dataLen) + " bytes");
        }
    }
}

void ResponseReceiver::notifyAboutError(std::string details)
{
    notify(&ResponsesListener::onResponseParseError, std::move(details));
}

std::string_view ResponseReceiver::logCategory() const
{
    static const std::string_view category("response_interceptor");
    return category;
}

std::optional<livekit::SignalResponse> ResponseReceiver::
    parseResponse(const void* data, size_t dataLen) const
{
    return protoFromBytes<livekit::SignalResponse>(data, dataLen);
}

std::optional<livekit::DataPacket>  ResponseReceiver::
    parseDataPacket(const void* data, size_t dataLen) const
{
    return protoFromBytes<livekit::DataPacket>(data, dataLen);
}

template <class Method, typename... Args>
void ResponseReceiver::notify(const Method& method, Args&&... args) const
{
    _listener.invoke(method, std::forward<Args>(args)...);
}

template <class Method, class TLiveKitType>
void ResponseReceiver::signal(const Method& method, TLiveKitType sig,
                              std::string typeName) const
{
    if (typeName.empty()) {
        typeName = marshalledTypeName<TLiveKitType>();
    }
    logVerbose(formatVerboseMsg(typeName));
    notify(method, _marshaller.map(std::move(sig)));
}

void ResponseReceiver::handle(livekit::JoinResponse response) const
{
    signal(&ResponsesListener::onJoin, std::move(response));
}

void ResponseReceiver::handle(livekit::SessionDescription desc, bool offer) const
{
    auto typeName = marshalledTypeName<livekit::SessionDescription>();
    if (offer) {
        signal(&ResponsesListener::onOffer, std::move(desc), typeName + "/offer");
    }
    else {
        signal(&ResponsesListener::onAnswer, std::move(desc), typeName + "/answer");
    }
}

void ResponseReceiver::handle(livekit::TrickleRequest request) const
{
    signal(&ResponsesListener::onTrickle, std::move(request));
}

void ResponseReceiver::handle(livekit::ParticipantUpdate update) const
{
    signal(&ResponsesListener::onUpdate, std::move(update));
}

void ResponseReceiver::handle(livekit::TrackPublishedResponse response) const
{
    signal(&ResponsesListener::onTrackPublished, std::move(response));
}

void ResponseReceiver::handle(livekit::LeaveRequest request) const
{
    signal(&ResponsesListener::onLeave, std::move(request));
}

void ResponseReceiver::handle(livekit::MuteTrackRequest request) const
{
    signal(&ResponsesListener::onMute, std::move(request));
}

void ResponseReceiver::handle(livekit::SpeakersChanged changed) const
{
    signal(&ResponsesListener::onSpeakersChanged, std::move(changed));
}

void ResponseReceiver::handle(livekit::RoomUpdate update) const
{
    signal(&ResponsesListener::onRoomUpdate, std::move(update));
}

void ResponseReceiver::handle(livekit::ConnectionQualityUpdate update) const
{
    signal(&ResponsesListener::onConnectionQuality, std::move(update));
}

void ResponseReceiver::handle(livekit::StreamStateUpdate update) const
{
    signal(&ResponsesListener::onStreamStateUpdate, std::move(update));
}

void ResponseReceiver::handle(livekit::SubscribedQualityUpdate update) const
{
    signal(&ResponsesListener::onSubscribedQualityUpdate, std::move(update));
}

void ResponseReceiver::handle(livekit::SubscriptionPermissionUpdate update) const
{
    signal(&ResponsesListener::onSubscriptionPermission, std::move(update));
}

void ResponseReceiver::handle(livekit::TrackUnpublishedResponse response) const
{
    signal(&ResponsesListener::onTrackUnpublished, std::move(response));
}

void ResponseReceiver::handle(livekit::ReconnectResponse response) const
{
    signal(&ResponsesListener::onReconnect, std::move(response));
}

void ResponseReceiver::handle(livekit::SubscriptionResponse response) const
{
    signal(&ResponsesListener::onSubscriptionResponse, std::move(response));
}

void ResponseReceiver::handle(livekit::RequestResponse response) const
{
    signal(&ResponsesListener::onRequestResponse, std::move(response));
}

void ResponseReceiver::handle(livekit::TrackSubscribed subscribed) const
{
    signal(&ResponsesListener::onTrackSubscribed, std::move(subscribed));
}

void ResponseReceiver::handle(livekit::Pong pong) const
{
    signal(&ResponsesListener::onPong, std::move(pong));
}

void ResponseReceiver::handle(livekit::DataPacket packet) const
{
    signal(&ResponsesListener::onDataPacket, std::move(packet));
}

void ResponseReceiver::handle(livekit::RoomMovedResponse response) const
{
    signal(&ResponsesListener::onRoomMovedResponse, std::move(response));
}

} // namespace LiveKitCpp
