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
#include "ProtectedObj.h"
#include "Listeners.h"
#include "MemoryBlock.h"
#include "core/ResponseReceiver.h"

namespace LiveKitCpp
{

class SignalClient::Impl
{
public:
    Impl(uint64_t id, CommandSender* commandSender);
    State transportState() const noexcept;
    bool changeTransportState(State state);
    void notifyAboutTransportError(const std::string& error);
    void addListener(SignalTransportListener* listener);
    void removeListener(SignalTransportListener* listener);
private:
    const uint64_t _id;
    CommandSender* const _commandSender;
    Listeners<SignalTransportListener*> _listeners;
    ProtectedObj<State> _transportState = State::Disconnected;
};

SignalClient::SignalClient(CommandSender* commandSender)
    : _impl(std::make_unique<Impl>(id(), commandSender))
    , _responseReceiver(std::make_unique<ResponseReceiver>(id()))
{
}

SignalClient::~SignalClient()
{
}

void SignalClient::addListener(SignalTransportListener* listener)
{
    _impl->addListener(listener);
}

void SignalClient::addListener(SignalServerListener* listener)
{
    _responseReceiver->addListener(listener);
}

void SignalClient::removeListener(SignalTransportListener* listener)
{
    _impl->removeListener(listener);
}

void SignalClient::removeListener(SignalServerListener* listener)
{
    _responseReceiver->removeListener(listener);
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

bool SignalClient::changeTransportState(State state)
{
    return _impl->changeTransportState(state);
}

void SignalClient::handleServerProtobufMessage(const std::shared_ptr<const MemoryBlock>& message)
{
    if (message) {
        handleServerProtobufMessage(message->data(), message->size());
    }
}

void SignalClient::handleServerProtobufMessage(const void* message, size_t len)
{
    _responseReceiver->parseBinary(message, len);
}

void SignalClient::notifyAboutTransportError(const std::string& error)
{
    _impl->notifyAboutTransportError(error);
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
        _listeners.invokeMethod(&SignalTransportListener::onTransportStateChanged, _id, state);
    }
    return accepted;
}

void SignalClient::Impl::notifyAboutTransportError(const std::string& error)
{
    _listeners.invokeMethod(&SignalTransportListener::onTransportError, _id, error);
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
