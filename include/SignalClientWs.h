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

class LIVEKIT_CLIENT_API SignalClientWs : public SignalClient,
                                          private CommandSender
{
    class Listener;
    struct Impl;
public:
    SignalClientWs(std::unique_ptr<Websocket::EndPoint> socket,
                   Bricks::Logger* logger = nullptr);
    ~SignalClientWs() final;
    std::string host() const noexcept;
    std::string authToken() const noexcept;
    std::string participantSid() const noexcept;
    bool autoSubscribe() const noexcept;
    bool adaptiveStream() const noexcept;
    ReconnectMode reconnectMode() const noexcept;
    int protocolVersion() const noexcept;
    void setAutoSubscribe(bool autoSubscribe);
    void setAdaptiveStream(bool adaptiveStream);
    void setReconnectMode(ReconnectMode reconnectMode);
    void setProtocolVersion(int protocolVersion);
    void setHost(std::string host);
    void setAuthToken(std::string authToken);
    void setParticipantSid(std::string participantSid);
    // impl. of SignalClient
    bool connect() final;
    void disconnect() final;
    bool ping();
private:
    void updateState(Websocket::State state);
    // impl. of CommandSender
    bool sendBinary(const Bricks::Blob& binary) final;
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
