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
#include "Blob.h"
#include "SafeObjAliases.h"
#include "Listener.h"
#include "NetworkType.h"
#include "WebsocketEndPoint.h"
#include "WebsocketError.h"
#include "WebsocketState.h"
#include "WebsocketListener.h"
#include "WebsocketOptions.h"
#include "Utils.h"
#include <atomic>

namespace {

using namespace LiveKitCpp;

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

struct SignalClientWs::UrlData
{
    Bricks::SafeObj<std::string> _host;
    Bricks::SafeObj<std::string> _authToken;
    Bricks::SafeObj<std::string> _participantSid;
    Bricks::SafeObj<std::string> _publish;
    Bricks::SafeOptional<ClientInfo> _clientInfo;
    std::atomic_bool _autoSubscribe = true;
    std::atomic_bool _adaptiveStream = true;
    Websocket::Options buildOptions() const;
};

class SignalClientWs::Listener : public Websocket::Listener
{
public:
    Listener() = default;
    void setOwner(SignalClientWs* owner) { _owner = owner; }
private:
    // impl. of WebsocketListener
    void onError(uint64_t, uint64_t, const Websocket::Error& error) final;
    void onStateChanged(uint64_t, uint64_t, Websocket::State state) final;
    void onBinaryMessage(uint64_t, uint64_t, const Bricks::Blob& message) final;
private:
    Bricks::Listener<SignalClientWs*> _owner = nullptr;
};

SignalClientWs::SignalClientWs(std::unique_ptr<Websocket::EndPoint> socket,
                               Bricks::Logger* logger)
    : SignalClient(this, logger)
    , _urlData(std::make_unique<UrlData>())
    , _listener(std::make_shared<Listener>())
    , _socket(std::move(socket))
{
    if (_socket) {
        _listener->setOwner(this);
        _socket->setListener(_listener);
    }
}

SignalClientWs::~SignalClientWs()
{
    SignalClientWs::disconnect();
    if (_socket) {
        _socket->resetListener();
        _listener->setOwner(nullptr);
    }
}

std::string SignalClientWs::host() const noexcept
{
    return _urlData->_host();
}

std::string SignalClientWs::authToken() const noexcept
{
    return _urlData->_authToken();
}

std::string SignalClientWs::participantSid() const noexcept
{
    return _urlData->_participantSid();
}

std::string SignalClientWs::publish() const noexcept
{
    return _urlData->_publish();
}

bool SignalClientWs::autoSubscribe() const noexcept
{
    return _urlData->_autoSubscribe;
}

bool SignalClientWs::adaptiveStream() const noexcept
{
    return _urlData->_adaptiveStream;
}

void SignalClientWs::setAutoSubscribe(bool autoSubscribe)
{
    if (exchange(autoSubscribe, _urlData->_autoSubscribe)) {
        logVerbose("auto subscribe policy has been changed");
    }
}

void SignalClientWs::setAdaptiveStream(bool adaptiveStream)
{
    if (exchange(adaptiveStream, _urlData->_adaptiveStream)) {
        logVerbose("adaptive stream policy has been changed");
    }
}

void SignalClientWs::setClientInfo(const std::optional<ClientInfo>& clientInfo)
{
    _urlData->_clientInfo(clientInfo);
    if (clientInfo && LIVEKIT_PROTOCOL_VERSION != clientInfo->_protocol) {
        logWarning("protocol version is not matched to default");
    }
}

void SignalClientWs::setHost(std::string host)
{
    _urlData->_host(std::move(host));
}

void SignalClientWs::setAuthToken(std::string authToken)
{
    _urlData->_authToken(std::move(authToken));
}

void SignalClientWs::setParticipantSid(std::string participantSid)
{
    _urlData->_participantSid(std::move(participantSid));
}

void SignalClientWs::setPublish(std::string publish)
{
    _urlData->_publish(std::move(publish));
}

bool SignalClientWs::connect()
{
    bool ok = false;
    if (_socket) {
        const auto result = changeTransportState(TransportState::Connecting);
        if (ChangeTransportStateResult::Changed == result) {
            ok = _socket->open(_urlData->buildOptions());
            if (!ok) {
                changeTransportState(TransportState::Disconnected);
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

bool SignalClientWs::ping()
{
    return _socket && _socket->ping();
}

void SignalClientWs::updateState(Websocket::State state)
{
    switch (state) {
        case Websocket::State::Connecting:
            changeTransportState(TransportState::Connecting);
            break;
        case Websocket::State::Connected:
            changeTransportState(TransportState::Connected);
            break;
        case Websocket::State::Disconnecting:
            changeTransportState(TransportState::Disconnecting);
            break;
        case Websocket::State::Disconnected:
            changeTransportState(TransportState::Disconnected);
            break;
    }
}

bool SignalClientWs::sendBinary(const Bricks::Blob& binary)
{
    return _socket && _socket->sendBinary(binary);
}

void SignalClientWs::Listener::onError(uint64_t, uint64_t, const Websocket::Error& error)
{
    _owner.invoke(&SignalClientWs::notifyAboutTransportError, Websocket::toString(error));
}

void SignalClientWs::Listener::onStateChanged(uint64_t, uint64_t, Websocket::State state)
{
    _owner.invoke(&SignalClientWs::updateState, state);
}

void SignalClientWs::Listener::onBinaryMessage(uint64_t, uint64_t,
                                               const Bricks::Blob& message)
{
    _owner.invoke(&SignalClientWs::handleServerProtobufMessage, message);
}

Websocket::Options SignalClientWs::UrlData::buildOptions() const
{
    Websocket::Options options;
    auto host = _host();
    const auto authToken = _authToken();
    if (!host.empty() && !authToken.empty()) {
        // see example in https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Support/Utils.swift#L138
        options._host = std::move(host);
        if ('/' != options._host.back()) {
            options._host += '/';
        }
        options._host += "rtc?access_token=" + authToken;
        options._host += urlQueryItem("auto_subscribe", _autoSubscribe.load());
        options._host += urlQueryItem("adaptive_stream", _adaptiveStream.load());
        options._host += urlQueryItem("publish", _publish());
        // only for quick-reconnect
        const auto participantSid = _participantSid();
        if (!participantSid.empty()) {
            options._host += urlQueryItem("reconnect", 1);
            options._host += urlQueryItem("sid", participantSid);
        }
        LOCK_READ_SAFE_OBJ(_clientInfo);
        if (const auto ci = _clientInfo.constRef()) {
            options._host += urlQueryItem("sdk", toString(ci->_sdk));
            options._host += urlQueryItem("version", ci->_version);
            options._host += urlQueryItem("protocol", ci->_protocol > 0 ?
                                          ci->_protocol : LIVEKIT_PROTOCOL_VERSION);
            options._host += urlQueryItem("os", ci->_os.empty() ?
                                          operatingSystemName() : ci->_os);
            options._host += urlQueryItem("os_version", ci->_osVersion.empty() ?
                                          operatingSystemVersion() : ci->_osVersion);
            options._host += urlQueryItem("device_model", ci->_deviceModel.empty() ?
                                          modelIdentifier() : ci->_deviceModel);
            auto network = ci->_network;
            if (network.empty()) {
                network = toString(activeNetworkType());
            }
            options._host += urlQueryItem("network", network);
        }
        else {
            options._host += urlQueryItem("sdk", toString(SDK::CPP));
            options._host += urlQueryItem("version", g_libraryVersion);
            options._host += urlQueryItem("protocol", LIVEKIT_PROTOCOL_VERSION);
            options._host += urlQueryItem("os", operatingSystemName());
            options._host += urlQueryItem("os_version", operatingSystemVersion());
            options._host += urlQueryItem("device_model", modelIdentifier());
            options._host += urlQueryItem("network", toString(activeNetworkType()));
        }
    }
    return options;
}

} // namespace LiveKitCpp
