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
#include "SignalTransportListener.h"
#include "SafeObj.h"
#include "Listener.h"
#include "ResponseInterceptor.h"
#include "RequestInterceptor.h"
#include "Utils.h"
#include "Blob.h"

namespace LiveKitCpp
{

class SignalClient::Impl
{
public:
    Impl(const SignalClient* client);
    TransportState transportState() const noexcept;
    ChangeTransportStateResult changeTransportState(TransportState state);
    void notifyAboutTransportError(std::string error);
    void setListener(SignalTransportListener* listener) { _listener = listener; }
private:
    const SignalClient* _client;
    Bricks::Listener<SignalTransportListener*> _listener;
    Bricks::SafeObj<TransportState> _transportState = TransportState::Disconnected;
};

SignalClient::SignalClient(CommandSender* commandSender, Bricks::Logger* logger)
    :  Bricks::LoggableR<>(logger)
    , _impl(std::make_unique<Impl>(this))
    , _responseReceiver(std::make_unique<ResponseInterceptor>(id(), logger))
    , _requestSender(std::make_unique<RequestInterceptor>(commandSender, logger))
{
}

SignalClient::~SignalClient()
{
}

void SignalClient::setTransportListener(SignalTransportListener* listener)
{
    _impl->setListener(listener);
}

void SignalClient::setServerListener(SignalServerListener* listener)
{
    _responseReceiver->setListener(listener);
}

bool SignalClient::connect()
{
    return ChangeTransportStateResult::Rejected != changeTransportState(TransportState::Connected);
}

void SignalClient::disconnect()
{
    changeTransportState(TransportState::Disconnected);
}

TransportState SignalClient::transportState() const noexcept
{
    return _impl->transportState();
}

bool SignalClient::sendOffer(const SessionDescription& sdp) const
{
    return _requestSender->offer(sdp);
}

bool SignalClient::sendAnswer(const SessionDescription& sdp) const
{
    return _requestSender->answer(sdp);
}

bool SignalClient::sendTrickle(const TrickleRequest& request) const
{
    return _requestSender->trickle(request);
}

bool SignalClient::sendAddTrack(const AddTrackRequest& request) const
{
    return _requestSender->addTrack(request);
}

bool SignalClient::sendMuteTrack(const MuteTrackRequest& request) const
{
    return _requestSender->muteTrack(request);
}

bool SignalClient::sendSubscription(const UpdateSubscription& update) const
{
    return _requestSender->subscription(update);
}

bool SignalClient::sendTrackSettings(const UpdateTrackSettings& update) const
{
    return _requestSender->trackSettings(update);
}

bool SignalClient::sendLeave(const LeaveRequest& request) const
{
    return _requestSender->leave(request);
}

bool SignalClient::sendUpdateVideoLayers(const UpdateVideoLayers& update) const
{
    return _requestSender->updateVideoLayers(update);
}

bool SignalClient::sendSubscriptionPermission(const SubscriptionPermission& permission) const
{
    return _requestSender->subscriptionPermission(permission);
}

bool SignalClient::sendSyncState(const SyncState& state) const
{
    return _requestSender->syncState(state);
}

bool SignalClient::sendSimulate(const SimulateScenario& scenario) const
{
    return _requestSender->simulate(scenario);
}

bool SignalClient::sendUpdateMetadata(const UpdateParticipantMetadata& data) const
{
    return _requestSender->updateMetadata(data);
}

bool SignalClient::sendPingReq(const Ping& ping) const
{
    return _requestSender->pingReq(ping);
}

bool SignalClient::sendUpdateAudioTrack(const UpdateLocalAudioTrack& track) const
{
    return _requestSender->updateAudioTrack(track);
}

bool SignalClient::sendUpdateVideoTrack(const UpdateLocalVideoTrack& track) const
{
    return _requestSender->updateVideoTrack(track);
}

SignalClient::ChangeTransportStateResult SignalClient::changeTransportState(TransportState state)
{
    return _impl->changeTransportState(state);
}

void SignalClient::notifyAboutTransportError(std::string error)
{
    _impl->notifyAboutTransportError(std::move(error));
}

void SignalClient::handleServerProtobufMessage(const Bricks::Blob& message)
{
    if (message) {
        _responseReceiver->parseBinary(message.data(), message.size());
    }
}

std::string_view SignalClient::logCategory() const
{
    static const std::string_view category("signaling_client");
    return category;
}

SignalClient::Impl::Impl(const SignalClient* client)
    : _client(client)
{
}

TransportState SignalClient::Impl::transportState() const noexcept
{
    LOCK_READ_SAFE_OBJ(_transportState);
    return _transportState.constRef();
}

SignalClient::ChangeTransportStateResult SignalClient::Impl::changeTransportState(TransportState state)
{
    ChangeTransportStateResult result = ChangeTransportStateResult::Rejected;
    {
        LOCK_WRITE_SAFE_OBJ(_transportState);
        if (_transportState != state) {
            bool accepted = false;
            switch(_transportState.constRef()) {
                case TransportState::Connecting:
                    // any state is good
                    accepted = true;
                    break;
                case TransportState::Connected:
                    accepted = TransportState::Disconnecting == state || TransportState::Disconnected == state;
                    break;
                case TransportState::Disconnecting:
                    accepted = TransportState::Disconnected == state;
                    break;
                case TransportState::Disconnected:
                    accepted = TransportState::Connecting == state || TransportState::Connected == state;
                    break;
            }
            if (accepted) {
                if (_client->canLogVerbose()) {
                    _client->logVerbose(makeStateChangesString(_transportState, state));
                }
                _transportState = state;
                result = ChangeTransportStateResult::Changed;
            }
        }
        else {
            result = ChangeTransportStateResult::NotChanged;
        }
    }
    if (ChangeTransportStateResult::Changed == result) {
        _listener.invoke(&SignalTransportListener::onTransportStateChanged,
                         _client->id(), state);
    }
    return result;
}

void SignalClient::Impl::notifyAboutTransportError(std::string error)
{
    _listener.invoke(&SignalTransportListener::onTransportError,
                     _client->id(), std::move(error));
}

} // namespace LiveKitCpp
