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
#include "livekit_rtc.pb.h"
#include <optional>

using namespace livekit;

namespace {

template <class TProtoBuffResponseClass>
inline std::optional<TProtoBuffResponseClass> parseResponse(const void* data,
                                                            size_t dataLen) {
    if (data && dataLen) {
        TProtoBuffResponseClass response;
        if (response.ParseFromArray(data, int(dataLen))) {
            return response;
        }
    }
    return std::nullopt;
}

inline std::optional<SignalResponse> parseSignalResponse(const void* data,
                                                         size_t dataLen) {
    return parseResponse<SignalResponse>(data, dataLen);
}

}

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
    void handle(const JoinResponse& join);
    void handle(const SessionDescription& desc, bool offer);
    void handle(const TrickleRequest& trickle);
    void handle(const ParticipantUpdate& participantUpdate);
    void handle(const TrackPublishedResponse& trackPublished);
    void handle(const LeaveRequest& leave);
    void handle(const MuteTrackRequest& muteTrack);
    void handle(const SpeakersChanged& speakersChanged);
    void handle(const RoomUpdate& roomtUpdate);
    void handle(const ConnectionQualityUpdate& qualityUpdate);
    void handle(const StreamStateUpdate& stateUpdate);
    void handle(const SubscribedQualityUpdate& qualityUpdate);
    void handle(const SubscriptionPermissionUpdate& permissionUpdate);
    void handle(const TrackUnpublishedResponse& trackUnpublished);
    void handle(const ReconnectResponse& reconnect);
    void handle(const SubscriptionResponse& subscription);
    void handle(const RequestResponse& request);
    void handle(const TrackSubscribed& trackSubscribed);
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
            case SignalResponse::kJoin:
                handle(response->join());
                break;
            case SignalResponse::kAnswer:
                handle(response->answer(), false);
                break;
            case SignalResponse::kOffer:
                handle(response->offer(), true);
                break;
            case SignalResponse::kTrickle:
                handle(response->trickle());
                break;
            case SignalResponse::kUpdate:
                handle(response->update());
                break;
            case SignalResponse::kTrackPublished:
                handle(response->track_published());
                break;
            case SignalResponse::kLeave:
                handle(response->leave());
                break;
            case SignalResponse::kMute:
                handle(response->mute());
                break;
            case SignalResponse::kSpeakersChanged:
                handle(response->speakers_changed());
                break;
            case SignalResponse::kRoomUpdate:
                handle(response->room_update());
                break;
            case SignalResponse::kConnectionQuality:
                handle(response->connection_quality());
                break;
            case SignalResponse::kStreamStateUpdate:
                handle(response->stream_state_update());
                break;
            case SignalResponse::kSubscribedQualityUpdate:
                handle(response->subscribed_quality_update());
                break;
            case SignalResponse::kSubscriptionPermissionUpdate:
                handle(response->subscription_permission_update());
                break;
            case SignalResponse::kRefreshToken:
                response->refresh_token(); // std::string
                break;
            case SignalResponse::kTrackUnpublished:
                handle(response->track_unpublished());
                break;
            case SignalResponse::kPong:
                response->pong(); // int64_t
                break;
            case SignalResponse::kReconnect:
                handle(response->reconnect());
                break;
            case SignalResponse::kPongResp:
                response->pong(); // int64_t
                break;
            case SignalResponse::kSubscriptionResponse:
                handle(response->subscription_response());
                break;
            case SignalResponse::kRequestResponse:
                handle(response->request_response());
                break;
            case SignalResponse::kTrackSubscribed:
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

void SignalClient::Impl::handle(const JoinResponse& join)
{
    
}

void SignalClient::Impl::handle(const SessionDescription& desc, bool offer)
{
    
}

void SignalClient::Impl::handle(const TrickleRequest& trickle)
{
    
}

void SignalClient::Impl::handle(const ParticipantUpdate& participantUpdate)
{
    
}

void SignalClient::Impl::handle(const TrackPublishedResponse& trackPublished)
{
    
}

void SignalClient::Impl::handle(const LeaveRequest& leave)
{
    
}

void SignalClient::Impl::handle(const MuteTrackRequest& muteTrack)
{
    
}

void SignalClient::Impl::handle(const SpeakersChanged& speakersChanged)
{
    
}

void SignalClient::Impl::handle(const RoomUpdate& roomtUpdate)
{
    
}

void SignalClient::Impl::handle(const ConnectionQualityUpdate& qualityUpdate)
{
    
}

void SignalClient::Impl::handle(const StreamStateUpdate& stateUpdate)
{
    
}

void SignalClient::Impl::handle(const SubscribedQualityUpdate& qualityUpdate)
{
    
}

void SignalClient::Impl::handle(const SubscriptionPermissionUpdate& permissionUpdate)
{
    
}

void SignalClient::Impl::handle(const TrackUnpublishedResponse& trackUnpublished)
{
    
}

void SignalClient::Impl::handle(const ReconnectResponse& reconnect)
{
    
}

void SignalClient::Impl::handle(const SubscriptionResponse& subscription)
{
    
}

void SignalClient::Impl::handle(const RequestResponse& request)
{
    
}

void SignalClient::Impl::handle(const TrackSubscribed& trackSubscribed)
{
    
}

} // namespace LiveKitCpp
