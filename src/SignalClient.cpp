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
#include "Listeners.h"
#include "ResponseReceiver.h"
#include "RequestSender.h"
#include "Loggable.h"

namespace LiveKitCpp
{

class SignalClient::Impl : public Bricks::LoggableR<>
{
public:
    Impl(uint64_t id, Bricks::Logger* logger = nullptr);
    State transportState() const noexcept;
    ChangeTransportStateResult changeTransportState(State state);
    void notifyAboutTransportError(const std::string& error);
    void addListener(SignalTransportListener* listener);
    void removeListener(SignalTransportListener* listener);
private:
    const uint64_t _id;
    Bricks::Listeners<SignalTransportListener*> _listeners;
    Bricks::SafeObj<State> _transportState = State::Disconnected;
};

SignalClient::SignalClient(CommandSender* commandSender, Bricks::Logger* logger)
    : _impl(std::make_unique<Impl>(id(), logger))
    , _responseReceiver(std::make_unique<ResponseReceiver>(id(), logger))
    , _requestSender(std::make_unique<RequestSender>(commandSender, logger))
{
}

SignalClient::~SignalClient()
{
}

void SignalClient::addTransportListener(SignalTransportListener* listener)
{
    _impl->addListener(listener);
}

void SignalClient::addServerListener(SignalServerListener* listener)
{
    _responseReceiver->addListener(listener);
}

void SignalClient::removeTransportListener(SignalTransportListener* listener)
{
    _impl->removeListener(listener);
}

void SignalClient::removeServerListener(SignalServerListener* listener)
{
    _responseReceiver->removeListener(listener);
}

bool SignalClient::connect()
{
    return ChangeTransportStateResult::Rejected != changeTransportState(State::Connected);
}

void SignalClient::disconnect()
{
    changeTransportState(State::Disconnected);
}

State SignalClient::transportState() const noexcept
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

std::string_view SignalClient::defaultLogCategory()
{
    static const std::string_view category("SignalingClient");
    return category;
}

SignalClient::ChangeTransportStateResult SignalClient::changeTransportState(State state)
{
    return _impl->changeTransportState(state);
}

void SignalClient::handleServerProtobufMessage(const void* message, size_t len)
{
    _responseReceiver->parseBinary(message, len);
}

void SignalClient::notifyAboutTransportError(const std::string& error)
{
    _impl->notifyAboutTransportError(error);
}

bool SignalClient::canLog(Bricks::LoggingSeverity severity) const
{
    return _impl->canLog(severity);
}

void SignalClient::log(Bricks::LoggingSeverity severity,
                       std::string_view message,
                       std::string_view category)
{
    _impl->log(severity, message, category);
}

SignalClient::Impl::Impl(uint64_t id, Logger* logger)
    : Bricks::LoggableR<>(logger)
    , _id(id)
{
}

State SignalClient::Impl::transportState() const noexcept
{
    LOCK_READ_SAFE_OBJ(_transportState);
    return _transportState.constRef();
}

SignalClient::ChangeTransportStateResult SignalClient::Impl::changeTransportState(State state)
{
    ChangeTransportStateResult result = ChangeTransportStateResult::Rejected;
    {
        LOCK_WRITE_SAFE_OBJ(_transportState);
        if (_transportState != state) {
            bool accepted = false;
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
                if (canLogVerbose()) {
                    // TODO: add obj [ID] logging
                    logVerbose("State changed from '" +
                               std::string(toString(_transportState)) +
                               "' to '" +
                               std::string(toString(state)) + "'",
                               defaultLogCategory());
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
        _listeners.invoke(&SignalTransportListener::onTransportStateChanged, _id, state);
    }
    return result;
}

void SignalClient::Impl::notifyAboutTransportError(const std::string& error)
{
    _listeners.invoke(&SignalTransportListener::onTransportError, _id, error);
}

void SignalClient::Impl::addListener(SignalTransportListener* listener)
{
    _listeners.add(listener);
}

void SignalClient::Impl::removeListener(SignalTransportListener* listener)
{
    _listeners.remove(listener);
}

} // namespace LiveKitCpp
