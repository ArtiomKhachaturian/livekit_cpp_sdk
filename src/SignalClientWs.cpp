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
#include "SafeObj.h"
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

struct UrlData
{
    std::string _host;
    std::string _authToken;
    std::string _participantSid;
    std::string _publish;
    bool _autoSubscribe = true;
    bool _adaptiveStream = true;
    std::optional<LiveKitCpp::ClientInfo> _clientInfo;
    LiveKitCpp::ReconnectMode _reconnectMode = LiveKitCpp::ReconnectMode::None;
};

}

namespace LiveKitCpp
{

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

struct SignalClientWs::Impl
{
    Impl(std::unique_ptr<Websocket::EndPoint> socket, SignalClientWs* owner);
    ~Impl();
    Websocket::Options buildOptions() const;
public:
    const std::shared_ptr<Listener> _listener;
    const std::unique_ptr<Websocket::EndPoint> _socket;
    Bricks::SafeObj<UrlData> _urlData;
};

SignalClientWs::SignalClientWs(std::unique_ptr<Websocket::EndPoint> socket,
                               Bricks::Logger* logger)
    : SignalClient(this, logger)
    , _impl(std::make_unique<Impl>(std::move(socket), this))
{
}

SignalClientWs::~SignalClientWs()
{
}

std::string SignalClientWs::host() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_host;
}

std::string SignalClientWs::authToken() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_authToken;
}

std::string SignalClientWs::participantSid() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_participantSid;
}

std::string SignalClientWs::publish() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_publish;
}

bool SignalClientWs::autoSubscribe() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_autoSubscribe;
}

bool SignalClientWs::adaptiveStream() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_adaptiveStream;
}

ReconnectMode SignalClientWs::reconnectMode() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_reconnectMode;
}

void SignalClientWs::setAutoSubscribe(bool autoSubscribe)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
        if (autoSubscribe != _impl->_urlData->_autoSubscribe) {
            _impl->_urlData->_autoSubscribe = autoSubscribe;
            changed = true;
        }
    }
    if (changed) {
        logVerbose("Auto subscribe policy has been changed");
    }
}

void SignalClientWs::setAdaptiveStream(bool adaptiveStream)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
        if (adaptiveStream != _impl->_urlData->_adaptiveStream) {
            _impl->_urlData->_adaptiveStream = adaptiveStream;
            changed = true;
        }
    }
    if (changed) {
        logVerbose("Adaptive stream policy has been changed");
    }
}

void SignalClientWs::setReconnectMode(ReconnectMode reconnectMode)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
        if (reconnectMode != _impl->_urlData->_reconnectMode) {
            _impl->_urlData->_reconnectMode = reconnectMode;
            changed = true;
        }
    }
    if (changed) {
        logVerbose("Reconnect mode has been changed");
    }
}

void SignalClientWs::setClientInfo(const std::optional<ClientInfo>& clientInfo)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
    _impl->_urlData->_clientInfo = clientInfo;
    if (clientInfo && LIVEKIT_PROTOCOL_VERSION != clientInfo->_protocol) {
        logWarning("protocol version is not matched to default");
    }
}

void SignalClientWs::setHost(std::string host)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
    _impl->_urlData->_host = std::move(host);
}

void SignalClientWs::setAuthToken(std::string authToken)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
    _impl->_urlData->_authToken = std::move(authToken);
}

void SignalClientWs::setParticipantSid(std::string participantSid)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
    _impl->_urlData->_participantSid = std::move(participantSid);
}

void SignalClientWs::setPublish(std::string publish)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
    _impl->_urlData->_publish = std::move(publish);
}

bool SignalClientWs::connect()
{
    bool ok = false;
    if (_impl->_socket) {
        const auto result = changeTransportState(TransportState::Connecting);
        if (ChangeTransportStateResult::Changed == result) {
            ok = _impl->_socket->open(_impl->buildOptions());
            if (!ok) {
                changeTransportState(TransportState::Disconnected);
            }
        }
    }
    return ok;
}

void SignalClientWs::disconnect()
{
    if (_impl->_socket) {
        _impl->_socket->close();
    }
}

bool SignalClientWs::ping()
{
    return _impl->_socket && _impl->_socket->ping();
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
    if (_impl->_socket) {
        return _impl->_socket->sendBinary(binary);
    }
    return CommandSender::sendBinary(binary);
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

SignalClientWs::Impl::Impl(std::unique_ptr<Websocket::EndPoint> socket, SignalClientWs* owner)
    : _listener(std::make_shared<Listener>())
    , _socket(std::move(socket))
{
    if (_socket) {
        _listener->setOwner(owner);
        _socket->setListener(_listener);
    }
}

SignalClientWs::Impl::~Impl()
{
    if (_socket) {
        _socket->close();
        _socket->resetListener();
        _listener->setOwner(nullptr);
    }
}

Websocket::Options SignalClientWs::Impl::buildOptions() const
{
    Websocket::Options options;
    LOCK_READ_SAFE_OBJ(_urlData);
    if (!_urlData->_host.empty() && !_urlData->_authToken.empty()) {
        // see example in https://github.com/livekit/client-sdk-swift/blob/main/Sources/LiveKit/Support/Utils.swift#L138
        options._host = _urlData->_host;
        if ('/' != options._host.back()) {
            options._host += '/';
        }
        using namespace std::string_literals;
        options._host += "rtc?access_token=" + _urlData->_authToken;
        options._host += urlQueryItem("auto_subscribe", _urlData->_autoSubscribe);
        options._host += urlQueryItem("adaptive_stream", _urlData->_adaptiveStream);
        options._host += urlQueryItem("publish", _urlData->_publish);
        if (const auto& ci = _urlData->_clientInfo) {
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
        // only for quick-reconnect
        if (ReconnectMode::Quick == _urlData->_reconnectMode) {
            options._host += urlQueryItem("reconnect", 1);
            options._host += urlQueryItem("sid", _urlData->_participantSid);
        }
    }
    return options;
}

std::string toString(SDK sdk)
{
    switch (sdk) {
        case SDK::Unknown:
            break;
        case SDK::JS:
            return "JS";
        case SDK::Swift:
            return "SWIFT";
        case SDK::Android:
            return "ANDROID";
        case SDK::Flutter:
            return "FLUTTER";
        case SDK::GO:
            return "GO";
        case SDK::Unity:
            return "UNITY";
        case SDK::ReactNative:
            return "REACT_NATIVE";
        case SDK::Rust:
            return "RUST";
        case SDK::Python:
            return "PYTHON";
        case SDK::CPP:
            return "CPP";
        case SDK::UnityWeb:
            return "UNITY_WEB";
        case SDK::Node:
            return "NODE";
        default:
            break;
    }
    return "UNKNOWN";
}

} // namespace LiveKitCpp
