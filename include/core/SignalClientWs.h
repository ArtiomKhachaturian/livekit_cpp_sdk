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
#include "core/SignalClient.h"
#include "core/SignalClientListener.h"
#include "websocket/WebsocketListener.h"
#include <memory>

namespace LiveKitCpp
{

class Websocket;
class WebsocketFactory;

class SignalClientWs : public SignalClient,
                       private WebsocketListener,
                       private SignalClientListener
{
public:
    SignalClientWs(std::unique_ptr<Websocket> socket);
    SignalClientWs(const WebsocketFactory& socketFactory);
    ~SignalClientWs() final;
    const std::string& host() const noexcept;
    const std::string& authToken() const noexcept;
    bool autoSubscribe() const noexcept;
    bool adaptiveStream() const noexcept;
    void setHost(std::string host);
    void setAuthToken(std::string authToken);
    void setAutoSubscribe(bool autoSubscribe);
    void setAdaptiveStream(bool adaptiveStream);
    // impl. of SignalClient
    bool connect() final;
    void disconnect() final;
private:
    // impl. of WebsocketListener
    void onStateChanged(uint64_t socketId, uint64_t connectionId,
                        const std::string_view& host,
                        State state) final;
    void onTextMessageReceived(uint64_t socketId, uint64_t connectionId,
                               const std::string_view& message) final;
    void onBinaryMessageReceved(uint64_t socketId, uint64_t connectionId,
                                const std::shared_ptr<const MemoryBlock>& message) final;
    // impl. of SignalClientListener
    void onServerSignalParseError(uint64_t signalClientId) final;
private:
    const std::unique_ptr<Websocket> _socket;
    std::string _host;
    std::string _authToken;
    bool _autoSubscribe = true;
    bool _adaptiveStream = true;
};

} // namespace LiveKitCpp
