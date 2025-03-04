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
#include "WebsocketFailure.h"
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
    if (_socket && changeTransportState(State::Connecting)) {
        ok = _socket->open(buildWebsocketOptions());
        if (!ok) {
            changeTransportState(State::Disconnected);
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

WebsocketOptions SignalClientWs::buildWebsocketOptions() const
{
    WebsocketOptions options;
    if (!host().empty() && !authToken().empty()) {
        // see example in https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Support/Utils.swift#L138
        options._host = host();
        if ('/' != options._host.back()) {
            options._host += '/';
        }
        using namespace std::string_literals;
        options._host += "rtc?access_token=" + authToken();
        options._host += urlQueryItem("auto_subscribe", autoSubscribe());
        options._host += urlQueryItem("sdk", "cpp"s);
        options._host += urlQueryItem("version", g_libraryVersion);
        options._host += urlQueryItem("protocol", 15);
        options._host += urlQueryItem("adaptive_stream", adaptiveStream());
        options._host += urlQueryItem("os", operatingSystemName());
        options._host += urlQueryItem("os_version", operatingSystemVersion());
        options._host += urlQueryItem("device_model", modelIdentifier());
        // only for quick-reconnect
        if (ReconnectMode::Quick == reconnectMode()) {
            options._host += urlQueryItem("reconnect", 1);
            options._host += urlQueryItem("sid", participantSid());
        }
    }
    return options;
}

void SignalClientWs::onError(uint64_t socketId, uint64_t connectionId,
                             const std::string_view& host,
                             const WebsocketError& error)
{
    WebsocketListener::onError(socketId, connectionId, host, error);
    notifyAboutTransportError(toString(error));
}

void SignalClientWs::onStateChanged(uint64_t socketId, uint64_t connectionId,
                                    const std::string_view& host,
                                    State state)
{
    WebsocketListener::onStateChanged(socketId, connectionId, host, state);
    if (changeTransportState(state)) {
        // TODO: primary logic
    }
}

void SignalClientWs::onBinaryMessageReceved(uint64_t socketId, uint64_t connectionId,
                                            const std::shared_ptr<const MemoryBlock>& message)
{
    WebsocketListener::onBinaryMessageReceved(socketId, connectionId, message);
    handleServerProtobufMessage(message);
}

} // namespace LiveKitCpp
