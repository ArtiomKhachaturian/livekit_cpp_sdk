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
#include "State.h"

namespace LiveKitCpp
{

SignalClient::SignalClient(CommandSender* commandSender)
    : _commandSender(commandSender)
    , _state(State::Disconnected)
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
    LOCK_READ_PROTECTED_OBJ(_state);
    return _state.constRef();
}

void SignalClient::receiveBinary(const void* /*data*/, size_t /*dataLen*/)
{
}

void SignalClient::receiveText(const std::string_view& /*text*/)
{
}

bool SignalClient::changeState(State state)
{
    LOCK_WRITE_PROTECTED_OBJ(_state);
    if (_state != state) {
        bool accepted = false;
        switch(_state.constRef()) {
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
            _state = state;
        }
        return accepted;
    }
    return true;
}

} // namespace LiveKitCpp
