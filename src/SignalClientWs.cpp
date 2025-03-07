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
    bool _autoSubscribe = true;
    bool _adaptiveStream = true;
    LiveKitCpp::ReconnectMode _reconnectMode = LiveKitCpp::ReconnectMode::None;
    int _protocolVersion = LIVEKIT_PROTOCOL_VERSION;
};

}

namespace LiveKitCpp
{

class SignalClientWs::Listener : public Websocket::Listener
{
public:
    Listener(SignalClientWs* owner);
private:
    // impl. of WebsocketListener
    void onError(uint64_t socketId, uint64_t connectionId,
                 const Websocket::Error& error) final;
    void onStateChanged(uint64_t socketId, uint64_t connectionId,
                        Websocket::State state) final;
    void onBinaryMessage(uint64_t socketId, uint64_t connectionId,
                         const std::shared_ptr<Bricks::Blob>& message) final;
private:
    SignalClientWs* const _owner;
};

struct SignalClientWs::Impl
{
    Impl(std::unique_ptr<Websocket::EndPoint> socket, SignalClientWs* owner);
    ~Impl();
    Websocket::Options buildOptions() const;
public:
    const std::unique_ptr<Websocket::EndPoint> _socket;
    Listener _listener;
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

int SignalClientWs::protocolVersion() const noexcept
{
    LOCK_READ_SAFE_OBJ(_impl->_urlData);
    return _impl->_urlData->_protocolVersion;
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

void SignalClientWs::setProtocolVersion(int protocolVersion)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_impl->_urlData);
        if (protocolVersion != _impl->_urlData->_protocolVersion) {
            _impl->_urlData->_protocolVersion = protocolVersion;
            changed = true;
        }
    }
    if (changed) {
        logVerbose("Protocol version has been changed");
        if (LIVEKIT_PROTOCOL_VERSION != protocolVersion) {
            logWarning("Protocol version is not matched to default");
        }
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

bool SignalClientWs::connect()
{
    bool ok = false;
    if (_impl->_socket) {
        const auto result = changeTransportState(State::Connecting);
        if (ChangeTransportStateResult::Changed == result) {
            ok = _impl->_socket->open(_impl->buildOptions());
            if (!ok) {
                changeTransportState(State::Disconnected);
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

bool SignalClientWs::sendBinary(const std::shared_ptr<Bricks::Blob>& binary)
{
    if (_impl->_socket) {
        return _impl->_socket->sendBinary(binary);
    }
    return CommandSender::sendBinary(binary);
}

SignalClientWs::Listener::Listener(SignalClientWs* owner)
    : _owner(owner)
{
}

void SignalClientWs::Listener::onError(uint64_t socketId,
                                       uint64_t connectionId,
                                       const Websocket::Error& error)
{
    Websocket::Listener::onError(socketId, connectionId, error);
    _owner->notifyAboutTransportError(Websocket::toString(error));
}

void SignalClientWs::Listener::onStateChanged(uint64_t socketId,
                                              uint64_t connectionId,
                                              Websocket::State state)
{
    Websocket::Listener::onStateChanged(socketId, connectionId, state);
    _owner->updateState(state);
}

void SignalClientWs::Listener::onBinaryMessage(uint64_t socketId,
                                               uint64_t connectionId,
                                               const std::shared_ptr<Bricks::Blob>& message)
{
    Websocket::Listener::onBinaryMessage(socketId, connectionId, message);
    if (message) {
        _owner->handleServerProtobufMessage(message->data(), message->size());
    }
}

SignalClientWs::Impl::Impl(std::unique_ptr<Websocket::EndPoint> socket, SignalClientWs* owner)
    : _socket(std::move(socket))
    , _listener(owner)
{
    if (_socket) {
        _socket->addListener(&_listener);
    }
}

SignalClientWs::Impl::~Impl()
{
    if (_socket) {
        _socket->close();
        _socket->removeListener(&_listener);
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
        options._host += urlQueryItem("sdk", "cpp"s);
        options._host += urlQueryItem("version", g_libraryVersion);
        options._host += urlQueryItem("protocol", _urlData->_protocolVersion);
        options._host += urlQueryItem("adaptive_stream", _urlData->_adaptiveStream);
        options._host += urlQueryItem("os", operatingSystemName());
        options._host += urlQueryItem("os_version", operatingSystemVersion());
        options._host += urlQueryItem("device_model", modelIdentifier());
        // only for quick-reconnect
        if (ReconnectMode::Quick == _urlData->_reconnectMode) {
            options._host += urlQueryItem("reconnect", 1);
            options._host += urlQueryItem("sid", _urlData->_participantSid);
        }
    }
    return options;
}

} // namespace LiveKitCpp
