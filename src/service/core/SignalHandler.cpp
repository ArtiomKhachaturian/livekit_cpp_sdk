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
#include "SignalHandler.h"
#include "SignalParser.h"
#include "SignalServerListener.h"

namespace LiveKitCpp
{

SignalHandler::SignalHandler(uint64_t signalClientId)
    : _signalClientId(signalClientId)
{
}

void SignalHandler::parseBinary(const void* data, size_t dataLen)
{
    if (auto response = SignalParser::parse(data, dataLen)) {
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
                    notify(&SignalServerListener::onPong,
                           std::chrono::milliseconds{response->pong()},
                           std::chrono::milliseconds{});
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

void SignalHandler::addListener(SignalServerListener* listener)
{
    _listeners.add(listener);
}

void SignalHandler::removeListener(SignalServerListener* listener)
{
    _listeners.remove(listener);
}

template <class Method, typename... Args>
void SignalHandler::notify(const Method& method, Args&&... args) const
{
    _listeners.invokeMethod(method, _signalClientId, std::forward<Args>(args)...);
}

template <class Method, class TLiveKitType>
void SignalHandler::signal(const Method& method, const TLiveKitType& sig) const
{
    notify(method, SignalParser::from(sig));
}

void SignalHandler::handle(const livekit::JoinResponse& response) const
{
    signal(&SignalServerListener::onJoin, response);
}

void SignalHandler::handle(const livekit::SessionDescription& desc, bool offer) const
{
    if (offer) {
        notify(&SignalServerListener::onOffer, desc.type(), desc.sdp());
    }
    else {
        notify(&SignalServerListener::onAnswer, desc.type(), desc.sdp());
    }
}

void SignalHandler::handle(const livekit::TrickleRequest& request) const
{
    signal(&SignalServerListener::onTrickle, request);
}

void SignalHandler::handle(const livekit::ParticipantUpdate& update) const
{
    signal(&SignalServerListener::onParticipantUpdate, update);
}

void SignalHandler::handle(const livekit::TrackPublishedResponse& response) const
{
    signal(&SignalServerListener::onTrackPublished, response);
}

void SignalHandler::handle(const livekit::LeaveRequest& request) const
{
    signal(&SignalServerListener::onLeave, request);
}

void SignalHandler::handle(const livekit::MuteTrackRequest& request) const
{
    signal(&SignalServerListener::onMuteTrack, request);
}

void SignalHandler::handle(const livekit::SpeakersChanged& changed) const
{
    signal(&SignalServerListener::onSpeakersChanged, changed);
}

void SignalHandler::handle(const livekit::RoomUpdate& update) const
{
    signal(&SignalServerListener::onRoomUpdate, update);
}

void SignalHandler::handle(const livekit::ConnectionQualityUpdate& update) const
{
    signal(&SignalServerListener::onConnectionQualityUpdate, update);
}

void SignalHandler::handle(const livekit::StreamStateUpdate& update) const
{
    signal(&SignalServerListener::onStreamStateUpdate, update);
}

void SignalHandler::handle(const livekit::SubscribedQualityUpdate& update) const
{
    signal(&SignalServerListener::onSubscribedQualityUpdate, update);
}

void SignalHandler::handle(const livekit::SubscriptionPermissionUpdate& update) const
{
    signal(&SignalServerListener::onSubscriptionPermission, update);
}

void SignalHandler::handle(const livekit::TrackUnpublishedResponse& response) const
{
    signal(&SignalServerListener::onTrackUnpublished, response);
}

void SignalHandler::handle(const livekit::ReconnectResponse& response) const
{
    signal(&SignalServerListener::onReconnect, response);
}

void SignalHandler::handle(const livekit::SubscriptionResponse& response) const
{
    signal(&SignalServerListener::onSubscription, response);
}

void SignalHandler::handle(const livekit::RequestResponse& response) const
{
    signal(&SignalServerListener::onRequest, response);
}

void SignalHandler::handle(const livekit::TrackSubscribed& subscribed) const
{
    signal(&SignalServerListener::onTrackSubscribed, subscribed);
}

void SignalHandler::handle(const livekit::Pong& pong) const
{
    notify(&SignalServerListener::onPong,
           std::chrono::milliseconds{pong.timestamp()},
           std::chrono::milliseconds{pong.last_ping_timestamp()});
}

} // namespace LiveKitCpp
