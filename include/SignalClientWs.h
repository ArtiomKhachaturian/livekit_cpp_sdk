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
#include "CommandSender.h"
#include "ReconnectMode.h"
#include <memory>
#include <string>

namespace Websocket {
class EndPoint;
enum class State;
}

namespace LiveKitCpp
{

class LIVEKIT_CLIENT_API SignalClientWs : public SignalClient, private CommandSender
{
    class Listener;
public:
    SignalClientWs(std::unique_ptr<Websocket::EndPoint> socket, LogsReceiver* logger = nullptr);
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
    void updateState(Websocket::State state);
    // impl. of CommandSender
    bool sendBinary(const std::shared_ptr<Websocket::Blob>& binary) final;
private:
    const std::unique_ptr<Listener> _socketListener;
    const std::unique_ptr<Websocket::EndPoint> _socket;
    std::string _host;
    std::string _authToken;
    std::string _participantSid;
    bool _autoSubscribe = true;
    bool _adaptiveStream = true;
    ReconnectMode _reconnectMode = ReconnectMode::None;
};

} // namespace LiveKitCpp
