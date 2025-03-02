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
#ifdef USE_ZAPHOYD_TPP_SOCKETS
#include "WebsocketOptions.h"
#include <websocketpp/uri.hpp>

namespace LiveKitCpp
{

class WebsocketTppConfig
{
public:
    static WebsocketTppConfig create(WebsocketOptions options);
    WebsocketTppConfig() = default;
    WebsocketTppConfig(const WebsocketTppConfig&) = default;
    WebsocketTppConfig(WebsocketTppConfig&&) = default;
    const websocketpp::uri_ptr& uri() const noexcept { return _uri; }
    const WebsocketOptions& options() const noexcept { return _options; }
    bool valid() const noexcept { return nullptr != uri(); }
    bool secure() const noexcept;
    WebsocketTppConfig& operator = (const WebsocketTppConfig&) = default;
    WebsocketTppConfig& operator = (WebsocketTppConfig&&) = default;
    explicit operator bool () const noexcept { return valid(); }
    operator const WebsocketOptions& () const noexcept { return options(); }
    operator const websocketpp::uri_ptr& () const noexcept { return uri(); }
private:
    WebsocketTppConfig(websocketpp::uri_ptr uri, WebsocketOptions options);
private:
    websocketpp::uri_ptr _uri;
    WebsocketOptions _options;
};

} // namespace LiveKitCpp
#endif
