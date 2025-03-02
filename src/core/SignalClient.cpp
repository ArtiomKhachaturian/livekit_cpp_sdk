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
#include "ProtectedObj.h"

namespace LiveKitCpp
{

struct SignalClient::Impl
{
    Impl(CommandSender* commandSender);
    CommandSender* const _commandSender;
    ProtectedObj<State> _state = State::Disconnected;
};

SignalClient::SignalClient(CommandSender* commandSender)
    : _impl(std::make_unique<Impl>(commandSender))
{
}

SignalClient::~SignalClient()
{
}

bool SignalClient::connect()
{
    return changeState(State::Connected);
}

void SignalClient::disconnect()
{
    changeState(State::Disconnected);
}

State SignalClient::state() const noexcept
{
    LOCK_READ_PROTECTED_OBJ(_impl->_state);
    return _impl->_state.constRef();
}

void SignalClient::receiveBinary(const void* /*data*/, size_t /*dataLen*/)
{
}

void SignalClient::receiveText(const std::string_view& /*text*/)
{
}

bool SignalClient::changeState(State state)
{
    LOCK_WRITE_PROTECTED_OBJ(_impl->_state);
    if (_impl->_state != state) {
        bool accepted = false;
        switch(_impl->_state.constRef()) {
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
            _impl->_state = state;
        }
        return accepted;
    }
    return true;
}

SignalClient::Impl::Impl(CommandSender* commandSender)
    : _commandSender(commandSender)
{
}

} // namespace LiveKitCpp
