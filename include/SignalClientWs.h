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
#include "ReconnectMode.h"
#include "websocket/WebsocketListener.h"
#include "websocket/WebsocketOptions.h"
#include <memory>
#include <string>

namespace LiveKitCpp
{

class Websocket;
class WebsocketFactory;

class LIVEKIT_CLIENT_API SignalClientWs : public SignalClient,
                                          private WebsocketListener
{
public:
    SignalClientWs(std::unique_ptr<Websocket> socket, LogsReceiver* logger = nullptr);
    ~SignalClientWs() final;
    const std::string& host() const noexcept;
    const std::string& authToken() const noexcept;
    const std::string& participantSid() const noexcept;
    bool autoSubscribe() const noexcept;
    bool adaptiveStream() const noexcept;
    ReconnectMode reconnectMode() const noexcept;
    void setAutoSubscribe(bool autoSubscribe);
    void setAdaptiveStream(bool adaptiveStream);
    void setReconnectMode(ReconnectMode reconnectMode);
    void setHost(std::string host);
    void setAuthToken(std::string authToken);
    void setParticipantSid(std::string participantSid);
    // impl. of SignalClient
    bool connect() final;
    void disconnect() final;
private:
    WebsocketOptions buildWebsocketOptions() const;
    // impl. of WebsocketListener
    void onError(uint64_t socketId, uint64_t connectionId,
                 const std::string_view& host,
                 const WebsocketError& error) final;
    void onStateChanged(uint64_t socketId, uint64_t connectionId,
                        const std::string_view& host,
                        State state) final;
    void onBinaryMessageReceved(uint64_t socketId, uint64_t connectionId,
                                const std::shared_ptr<const MemoryBlock>& message) final;
private:
    const std::unique_ptr<Websocket> _socket;
    std::string _host;
    std::string _authToken;
    std::string _participantSid;
    bool _autoSubscribe = true;
    bool _adaptiveStream = true;
    ReconnectMode _reconnectMode = ReconnectMode::None;
};

} // namespace LiveKitCpp
