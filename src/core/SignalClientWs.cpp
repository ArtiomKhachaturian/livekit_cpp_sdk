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
#include "WebsocketOptions.h"
#include "WebsocketFailure.h"

namespace {

inline std::string urlQueryItem(const std::string& key, const std::string& val) {
    return "&" + key + "=" + val;
}

inline std::string urlQueryItem(const std::string& key, int val) {
    return urlQueryItem(key, std::to_string(val));
}

inline std::string urlQueryItem(const std::string& key, bool val) {
    return urlQueryItem(key, val ? 1 : 0);
}

inline LiveKitCpp::WebsocketOptions formatWsOptions(const std::string& host,
                                                    const std::string& authToken,
                                                    bool autoSubscribe,
                                                    bool adaptiveStream) {
    LiveKitCpp::WebsocketOptions options;
    if (!host.empty() && !authToken.empty()) {
        // see example in https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Support/Utils.swift#L138
        options._host = host;
        if ('/' != options._host.back()) {
            options._host += '/';
        }
        options._host += "rtc?access_token=" + authToken;
        options._host += urlQueryItem("auto_subscribe", autoSubscribe);
        options._host += urlQueryItem("sdk", "cpp");
        options._host += urlQueryItem("version", "2.9.5");
        options._host += urlQueryItem("protocol", 15);
        options._host += urlQueryItem("adaptive_stream", adaptiveStream);
    }
    return options;
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

bool SignalClientWs::autoSubscribe() const noexcept
{
    return _autoSubscribe;
}

bool SignalClientWs::adaptiveStream() const noexcept
{
    return _adaptiveStream;
}

void SignalClientWs::setAutoSubscribe(bool autoSubscribe)
{
    _autoSubscribe = autoSubscribe;
}

void SignalClientWs::setAdaptiveStream(bool adaptiveStream)
{
    _adaptiveStream = adaptiveStream;
}

void SignalClientWs::setHost(std::string host)
{
    _host = std::move(host);
}

void SignalClientWs::setAuthToken(std::string authToken)
{
    _authToken = std::move(authToken);
}

bool SignalClientWs::connect()
{
    bool ok = false;
    if (_socket && changeTransportState(State::Connecting)) {
        ok = _socket->open(formatWsOptions(_host,
                                           _authToken,
                                           _autoSubscribe,
                                           _adaptiveStream));
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
