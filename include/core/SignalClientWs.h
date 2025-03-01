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
#pragma once // SignalClientWs.h
#include "SignalClient.h"
#include "WebsocketListener.h"
#include <memory>

namespace LiveKitCpp
{

class Websocket;
class WebsocketFactory;

class SignalClientWs : public SignalClient, private WebsocketListener
{
public:
    SignalClientWs(std::unique_ptr<Websocket> socket);
    SignalClientWs(const WebsocketFactory& socketFactory);
    ~SignalClientWs() final;
private:
    // impl. of WebsocketListener
    void onTextMessageReceived(uint64_t socketId, uint64_t connectionId,
                               const std::string_view& message) final;
    void onBinaryMessageReceved(uint64_t socketId, uint64_t connectionId,
                                const void* data, size_t dataLen) final;
private:
    const std::unique_ptr<Websocket> _socket;
};

} // namespace LiveKitCpp
