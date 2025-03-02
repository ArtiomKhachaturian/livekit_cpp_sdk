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
// limitations under the License.
#pragma once
#include "WebsocketTls.h"
#include <optional>
#include <unordered_map>

namespace LiveKitCpp
{

struct WebsocketOptions
{
    using Linger = std::pair<bool, uint16_t>;
public:
    WebsocketOptions() = default;
    WebsocketOptions(std::string host);
    WebsocketOptions(const WebsocketOptions&) = default;
    WebsocketOptions(WebsocketOptions&&) = default;
    WebsocketOptions& operator = (const WebsocketOptions&) = default;
    WebsocketOptions& operator = (WebsocketOptions&&) = default;
    void addBasicAuthHeader(const std::string& user, const std::string& password);
    void addBearerAuthHeader(const std::string& token);
    void addAuthHeader(std::string auth);
    // URL/URI
    std::string _host;
    // agent
    std::string _userAgent;
    // headers
    std::unordered_map<std::string, std::string> _extraHeaders;
    // options
    // SOL_SOCKET/SO_BROADCAST
    std::optional<bool> _broadcast;
    // SOL_SOCKET/SO_DONTROUTE
    std::optional<bool> _doNotRoute;
    // SOL_SOCKET/SO_KEEPALIVE
    std::optional<bool> _keepAlive;
    // SOL_SOCKET/SO_LINGER https://www.ibm.com/docs/en/cics-tg-multi/9.2?topic=settings-so-linger-setting
    std::optional<Linger> _linger;
    // SOL_SOCKET/SO_RCVBUF
    std::optional<uint32_t> _receiveBufferSize;
    // SOL_SOCKET/SO_RCVLOWAT
    std::optional<uint32_t> _receiveLowWatermark;
    // SOL_SOCKET/SO_REUSEADDR
    std::optional<bool> _reuseAddress;
    // SOL_SOCKET/SO_SNDBUF
    std::optional<uint32_t> _sendBufferSize;
    // SOL_SOCKET/SO_SNDLOWAT
    std::optional<uint32_t> _sendLowWatermark;
    // IPPROTO_TCP/TCP_NODELAY
    std::optional<bool> _tcpNoDelay;
    // SSL/TLS
    WebsocketTls _tls;
};

} // namespace LiveKitCpp
