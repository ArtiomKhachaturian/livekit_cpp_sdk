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
#include "core/SignalParser.h"

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
    void notify(const Method& method, Args&&... args) const;
    template <class Method, class TLiveKitType>
    void signal(const Method& method, const TLiveKitType& sig) const;
    void handle(const livekit::JoinResponse& response) const;
    void handle(const livekit::SessionDescription& desc, bool offer) const;
    void handle(const livekit::TrickleRequest& request) const;
    void handle(const livekit::ParticipantUpdate& update) const;
    void handle(const livekit::TrackPublishedResponse& response) const;
    void handle(const livekit::LeaveRequest& request) const;
    void handle(const livekit::MuteTrackRequest& request) const;
    void handle(const livekit::SpeakersChanged& changed) const;
    void handle(const livekit::RoomUpdate& update) const;
    void handle(const livekit::ConnectionQualityUpdate& update) const;
    void handle(const livekit::StreamStateUpdate& update) const;
    void handle(const livekit::SubscribedQualityUpdate& update) const;
    void handle(const livekit::SubscriptionPermissionUpdate& update) const;
    void handle(const livekit::TrackUnpublishedResponse& response) const;
    void handle(const livekit::ReconnectResponse& response) const;
    void handle(const livekit::SubscriptionResponse& response) const;
    void handle(const livekit::RequestResponse& response) const;
    void handle(const livekit::TrackSubscribed& subscribed) const;
    void handle(const livekit::Pong& pong) const;
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
        notify(&SignalClientListener::onTransportStateChanged, state);
    }
    return accepted;
}

void SignalClient::Impl::parseBinary(const void* data, size_t dataLen)
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
                    notify(&SignalClientListener::onRefreshToken, response->refresh_token());
                }
                break;
            case livekit::SignalResponse::kTrackUnpublished:
                handle(response->track_unpublished());
                break;
            case livekit::SignalResponse::kPong: // deprecated
                if (response->has_pong()) {
                    notify(&SignalClientListener::onPong,
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
        notify(&SignalClientListener::onServerResponseParseError);
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
void SignalClient::Impl::notify(const Method& method, Args&&... args) const
{
    _listeners.invokeMethod(method, _id, std::forward<Args>(args)...);
}

template <class Method, class TLiveKitType>
void SignalClient::Impl::signal(const Method& method, const TLiveKitType& sig) const
{
    notify(method, SignalParser::from(sig));
}

void SignalClient::Impl::handle(const livekit::JoinResponse& response) const
{
    signal(&SignalClientListener::onJoin, response);
}

void SignalClient::Impl::handle(const livekit::SessionDescription& desc, bool offer) const
{
    if (offer) {
        notify(&SignalClientListener::onOffer, desc.type(), desc.sdp());
    }
    else {
        notify(&SignalClientListener::onAnswer, desc.type(), desc.sdp());
    }
}

void SignalClient::Impl::handle(const livekit::TrickleRequest& request) const
{
    signal(&SignalClientListener::onTrickle, request);
}

void SignalClient::Impl::handle(const livekit::ParticipantUpdate& update) const
{
    signal(&SignalClientListener::onParticipantUpdate, update);
}

void SignalClient::Impl::handle(const livekit::TrackPublishedResponse& response) const
{
    signal(&SignalClientListener::onTrackPublished, response);
}

void SignalClient::Impl::handle(const livekit::LeaveRequest& request) const
{
    signal(&SignalClientListener::onLeave, request);
}

void SignalClient::Impl::handle(const livekit::MuteTrackRequest& request) const
{
    signal(&SignalClientListener::onMuteTrack, request);
}

void SignalClient::Impl::handle(const livekit::SpeakersChanged& changed) const
{
    signal(&SignalClientListener::onSpeakersChanged, changed);
}

void SignalClient::Impl::handle(const livekit::RoomUpdate& update) const
{
    signal(&SignalClientListener::onRoomUpdate, update);
}

void SignalClient::Impl::handle(const livekit::ConnectionQualityUpdate& update) const
{
    signal(&SignalClientListener::onConnectionQualityUpdate, update);
}

void SignalClient::Impl::handle(const livekit::StreamStateUpdate& update) const
{
    signal(&SignalClientListener::onStreamStateUpdate, update);
}

void SignalClient::Impl::handle(const livekit::SubscribedQualityUpdate& update) const
{
    signal(&SignalClientListener::onSubscribedQualityUpdate, update);
}

void SignalClient::Impl::handle(const livekit::SubscriptionPermissionUpdate& update) const
{
    
}

void SignalClient::Impl::handle(const livekit::TrackUnpublishedResponse& response) const
{
    signal(&SignalClientListener::onTrackUnpublished, response);
}

void SignalClient::Impl::handle(const livekit::ReconnectResponse& response) const
{
    signal(&SignalClientListener::onReconnect, response);
}

void SignalClient::Impl::handle(const livekit::SubscriptionResponse& response) const
{
    signal(&SignalClientListener::onSubscription, response);
}

void SignalClient::Impl::handle(const livekit::RequestResponse& response) const
{
    signal(&SignalClientListener::onRequest, response);
}

void SignalClient::Impl::handle(const livekit::TrackSubscribed& subscribed) const
{
    signal(&SignalClientListener::onTrackSubscribed, subscribed);
}

void SignalClient::Impl::handle(const livekit::Pong& pong) const
{
    notify(&SignalClientListener::onPong,
           std::chrono::milliseconds{pong.timestamp()},
           std::chrono::milliseconds{pong.last_ping_timestamp()});
}

} // namespace LiveKitCpp
