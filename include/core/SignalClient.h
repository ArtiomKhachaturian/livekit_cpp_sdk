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
#pragma once // SignalClient.h
#include "CommandReceiver.h"
#include "State.h"

namespace LiveKitCpp
{

class CommandSender;
class Websocket;
class WebsocketFactory;

class SignalClient : public CommandReceiver
{
    struct Impl;
public:
    SignalClient(CommandSender* commandSender);
    ~SignalClient() override;
    virtual bool connect();
    virtual void disconnect();
    State state() const noexcept;
    // impl. of CommandReceiver
    void receiveBinary(const void* data, size_t dataLen) final;
    void receiveText(const std::string_view& text) final;
protected:
    bool changeState(State state);
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
