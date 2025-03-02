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
    void invokeListener(const Method& method, Args&&... args) const;
    void handle(const livekit::JoinResponse& join);
    void handle(const livekit::SessionDescription& desc, bool offer);
    void handle(const livekit::TrickleRequest& trickle);
    void handle(const livekit::ParticipantUpdate& participantUpdate);
    void handle(const livekit::TrackPublishedResponse& published);
    void handle(const livekit::LeaveRequest& leave);
    void handle(const livekit::MuteTrackRequest& muteTrack);
    void handle(const livekit::SpeakersChanged& changed);
    void handle(const livekit::RoomUpdate& update);
    void handle(const livekit::ConnectionQualityUpdate& qualityUpdate);
    void handle(const livekit::StreamStateUpdate& stateUpdate);
    void handle(const livekit::SubscribedQualityUpdate& qualityUpdate);
    void handle(const livekit::SubscriptionPermissionUpdate& permissionUpdate);
    void handle(const livekit::TrackUnpublishedResponse& unpublished);
    void handle(const livekit::ReconnectResponse& reconnect);
    void handle(const livekit::SubscriptionResponse& subscription);
    void handle(const livekit::RequestResponse& request);
    void handle(const livekit::TrackSubscribed& trackSubscribed);
    void handle(const livekit::Pong& pong);
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
    invokeListener(&SignalClientListener::onJoin, SignalParser::from(join));
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
    invokeListener(&SignalClientListener::onTrickle, SignalParser::from(trickle));
}

void SignalClient::Impl::handle(const livekit::ParticipantUpdate& participantUpdate)
{
    invokeListener(&SignalClientListener::onParticipantUpdate, SignalParser::from(participantUpdate));
}

void SignalClient::Impl::handle(const livekit::TrackPublishedResponse& published)
{
    invokeListener(&SignalClientListener::onTrackPublished, SignalParser::from(published));
}

void SignalClient::Impl::handle(const livekit::LeaveRequest& leave)
{
    invokeListener(&SignalClientListener::onLeave, SignalParser::from(leave));
}

void SignalClient::Impl::handle(const livekit::MuteTrackRequest& muteTrack)
{
    invokeListener(&SignalClientListener::onMuteTrack, SignalParser::from(muteTrack));
}

void SignalClient::Impl::handle(const livekit::SpeakersChanged& changed)
{
    invokeListener(&SignalClientListener::onSpeakersChanged, SignalParser::from(changed));
}

void SignalClient::Impl::handle(const livekit::RoomUpdate& update)
{
    invokeListener(&SignalClientListener::onRoomUpdate, SignalParser::from(update));
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

void SignalClient::Impl::handle(const livekit::TrackUnpublishedResponse& unpublished)
{
    invokeListener(&SignalClientListener::onTrackUnpublished, SignalParser::from(unpublished));
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

} // namespace LiveKitCpp
