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
#include "MemoryBlock.h"
#include "WebsocketBlob.h"
#include "WebsocketEndPoint.h"
#include "WebsocketFailure.h"
#include "WebsocketState.h"
#include "WebsocketListener.h"
#include "WebsocketOptions.h"
#include "Utils.h"

namespace {

const std::string g_libraryVersion(PROJECT_VERSION);

inline std::string urlQueryItem(const std::string& key, const std::string& val) {
    if (!key.empty() && !val.empty()) {
        return "&" + key + "=" + val;
    }
    return {};
}

inline std::string urlQueryItem(const std::string& key, int val) {
    if (!key.empty()) {
        return urlQueryItem(key, std::to_string(val));
    }
    return {};
}

inline std::string urlQueryItem(const std::string& key, bool val) {
    if (!key.empty()) {
        return urlQueryItem(key, val ? 1 : 0);
    }
    return {};
}

}

namespace LiveKitCpp
{

class SignalClientWs::Listener : public Websocket::Listener
{
public:
    Listener(SignalClientWs* owner);
    Websocket::Options buildOptions() const;
private:
    // impl. of WebsocketListener
    void onError(uint64_t socketId, uint64_t connectionId,
                 const Websocket::Error& error) final;
    void onStateChanged(uint64_t socketId, uint64_t connectionId,
                        Websocket::State state) final;
    void onBinaryMessage(uint64_t socketId, uint64_t connectionId,
                         const std::shared_ptr<Websocket::Blob>& message) final;
private:
    SignalClientWs* const _owner;
};

SignalClientWs::SignalClientWs(std::unique_ptr<Websocket::EndPoint> socket, LogsReceiver* logger)
    : SignalClient(this, logger)
    , _socketListener(std::make_unique<Listener>(this))
    , _socket(std::move(socket))
{
    if (_socket) {
        _socket->addListener(_socketListener.get());
    }
}

SignalClientWs::~SignalClientWs()
{
    if (_socket) {
        _socket->close();
        _socket->removeListener(_socketListener.get());
    }
}

const std::string& SignalClientWs::host() const noexcept
{
    return _host;
}

const std::string& SignalClientWs::authToken() const noexcept
{
    return _authToken;
}

const std::string& SignalClientWs::participantSid() const noexcept
{
    return _participantSid;
}

bool SignalClientWs::autoSubscribe() const noexcept
{
    return _autoSubscribe;
}

bool SignalClientWs::adaptiveStream() const noexcept
{
    return _adaptiveStream;
}

ReconnectMode SignalClientWs::reconnectMode() const noexcept
{
    return _reconnectMode;
}

void SignalClientWs::setAutoSubscribe(bool autoSubscribe)
{
    _autoSubscribe = autoSubscribe;
}

void SignalClientWs::setAdaptiveStream(bool adaptiveStream)
{
    _adaptiveStream = adaptiveStream;
}

void SignalClientWs::setReconnectMode(ReconnectMode reconnectMode)
{
    _reconnectMode = reconnectMode;
}

void SignalClientWs::setHost(std::string host)
{
    _host = std::move(host);
}

void SignalClientWs::setAuthToken(std::string authToken)
{
    _authToken = std::move(authToken);
}

void SignalClientWs::setParticipantSid(std::string participantSid)
{
    _participantSid = std::move(participantSid);
}

bool SignalClientWs::connect()
{
    bool ok = false;
    if (_socket) {
        const auto result = changeTransportState(State::Connecting);
        if (ChangeTransportStateResult::Changed == result) {
            ok = _socket->open(_socketListener->buildOptions());
            if (!ok) {
                changeTransportState(State::Disconnected);
            }
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

void SignalClientWs::updateState(Websocket::State state)
{
    switch (state) {
        case Websocket::State::Connecting:
            changeTransportState(State::Connecting);
            break;
        case Websocket::State::Connected:
            changeTransportState(State::Connected);
            break;
        case Websocket::State::Disconnecting:
            changeTransportState(State::Disconnecting);
            break;
        case Websocket::State::Disconnected:
            changeTransportState(State::Disconnected);
            break;
    }
}

bool SignalClientWs::sendBinary(const std::shared_ptr<MemoryBlock>& binary)
{
    if (_socket) {
        return _socket->sendBinary(binary);
    }
    return CommandSender::sendBinary(binary);
}

SignalClientWs::Listener::Listener(SignalClientWs* owner)
    : _owner(owner)
{
}

Websocket::Options SignalClientWs::Listener::buildOptions() const
{
    Websocket::Options options;
    if (!_owner->host().empty() && !_owner->authToken().empty()) {
        // see example in https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Support/Utils.swift#L138
        options._host = _owner->host();
        if ('/' != options._host.back()) {
            options._host += '/';
        }
        using namespace std::string_literals;
        options._host += "rtc?access_token=" + _owner->authToken();
        options._host += urlQueryItem("auto_subscribe", _owner->autoSubscribe());
        options._host += urlQueryItem("sdk", "cpp"s);
        options._host += urlQueryItem("version", g_libraryVersion);
        options._host += urlQueryItem("protocol", 15);
        options._host += urlQueryItem("adaptive_stream", _owner->adaptiveStream());
        options._host += urlQueryItem("os", operatingSystemName());
        options._host += urlQueryItem("os_version", operatingSystemVersion());
        options._host += urlQueryItem("device_model", modelIdentifier());
        // only for quick-reconnect
        if (ReconnectMode::Quick == _owner->reconnectMode()) {
            options._host += urlQueryItem("reconnect", 1);
            options._host += urlQueryItem("sid", _owner->participantSid());
        }
    }
    return options;
}

void SignalClientWs::Listener::onError(uint64_t socketId,
                                       uint64_t connectionId,
                                       const Websocket::Error& error)
{
    Listener::onError(socketId, connectionId, error);
    //notifyAboutTransportError(toString(error));
}

void SignalClientWs::Listener::onStateChanged(uint64_t socketId,
                                              uint64_t connectionId,
                                              Websocket::State state)
{
    Listener::onStateChanged(socketId, connectionId, state);
    _owner->updateState(state);
}

void SignalClientWs::Listener::onBinaryMessage(uint64_t socketId,
                                               uint64_t connectionId,
                                               const std::shared_ptr<Websocket::Blob>& message)
{
    Listener::onBinaryMessage(socketId, connectionId, message);
    if (message) {
        _owner->handleServerProtobufMessage(message->data(), message->size());
    }
}

} // namespace LiveKitCpp
