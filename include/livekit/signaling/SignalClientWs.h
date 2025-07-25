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
#include "livekit/signaling/SignalClient.h"
#include "livekit/signaling/CommandSender.h"
#include "livekit/signaling/sfu/ClientInfo.h"
#include "livekit/signaling/TransportState.h"
#include <memory>
#include <optional>
#include <string>

namespace Websocket {
class EndPoint;
enum class State;
}

namespace LiveKitCpp
{

class SignalTransportListener;

class LIVEKIT_SIGNALING_API SignalClientWs : public SignalClient,
                                             private CommandSender
{
    class Listener;
    struct Impl;
    struct UrlData;
public:
    SignalClientWs(std::unique_ptr<Websocket::EndPoint> socket,
                   Bricks::Logger* logger = nullptr);
    ~SignalClientWs() final;
    void setTransportListener(SignalTransportListener* listener = nullptr);
    TransportState transportState() const noexcept;
    std::string host() const noexcept;
    std::string authToken() const noexcept;
    std::string participantSid() const noexcept;
    std::string publish() const noexcept;
    bool autoSubscribe() const noexcept;
    bool adaptiveStream() const noexcept;
    void setAutoSubscribe(bool autoSubscribe);
    void setAdaptiveStream(bool adaptiveStream);
    void setClientInfo(const std::optional<ClientInfo>& clientInfo = {});
    void setHost(std::string host);
    void setAuthToken(std::string authToken);
    void setParticipantSid(std::string participantSid);
    void resetParticipantSid() { setParticipantSid({}); }
    void setPublish(std::string publish = {});
    bool connect();
    void disconnect();
    bool ping();
private:
    void updateState(Websocket::State state);
    // impl. of CommandSender
    bool sendBinary(const Bricks::Blob& binary) final;
private:
    const std::shared_ptr<UrlData> _urlData;
    const std::shared_ptr<Listener> _listener;
    const std::unique_ptr<Websocket::EndPoint> _socket;
};

} // namespace LiveKitCpp
