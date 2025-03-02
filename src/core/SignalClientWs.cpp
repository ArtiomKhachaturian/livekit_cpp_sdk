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
