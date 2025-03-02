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
#include "SignalClientWs.h"
#include "Websocket.h"
#include "WebsocketFactory.h"
#include "MemoryBlock.h"
#include "State.h"

namespace LiveKitCpp
{


SignalClientWs::SignalClientWs(std::unique_ptr<Websocket> socket)
    : SignalClient(socket.get())
    , _socket(std::move(socket))
{
    if (_socket) {
        _socket->addListener(this);
    }
}

SignalClientWs::SignalClientWs(const WebsocketFactory& socketFactory)
    : SignalClientWs(socketFactory.create())
{
}

SignalClientWs::~SignalClientWs()
{
    if (_socket) {
        _socket->close();
        _socket->removeListener(this);
    }
}

void SignalClientWs::setHost(std::string host)
{
    _socketOptions._host = std::move(host);
}

void SignalClientWs::setAuthentification(const std::string& user,
                                         const std::string& password)
{
    _socketOptions.addAuthHeader(user, password);
}

void SignalClientWs::setAuthentification(const std::string& authToken)
{
    _socketOptions.addAuthHeader(authToken);
}

bool SignalClientWs::connect()
{
    bool ok = false;
    if (_socket && changeState(State::Connecting)) {
        ok = _socket->open(_socketOptions);
        if (!ok) {
            changeState(State::Disconnected);
        }
    }
    return ok;
}

void SignalClientWs::disconnect()
{
    if (_socket) {
        _socket->close();
    }
}

void SignalClientWs::onStateChanged(uint64_t socketId, uint64_t connectionId,
                                    const std::string_view& host,
                                    State state)
{
    WebsocketListener::onStateChanged(socketId, connectionId, host, state);
    if (changeState(state)) {
        // TODO: primary logic
    }
}

void SignalClientWs::onTextMessageReceived(uint64_t socketId, uint64_t connectionId,
                                           const std::string_view& message)
{
    WebsocketListener::onTextMessageReceived(socketId, connectionId, message);
    receiveText(message);
}

void SignalClientWs::onBinaryMessageReceved(uint64_t socketId, uint64_t connectionId,
                                            const std::shared_ptr<const MemoryBlock>& message)
{
    WebsocketListener::onBinaryMessageReceved(socketId, connectionId, message);
    if (message) {
        receiveBinary(message->data(), message->size());
    }
}

} // namespace LiveKitCpp
