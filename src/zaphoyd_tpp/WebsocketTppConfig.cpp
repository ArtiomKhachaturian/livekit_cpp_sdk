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
#ifdef USE_ZAPHOYD_TPP_SOCKETS
#include "WebsocketTppConfig.h"

namespace LiveKitCpp
{

WebsocketTppConfig::WebsocketTppConfig(websocketpp::uri_ptr uri, WebsocketOptions options)
    : _uri(std::move(uri))
    , _options(std::move(options))
{
}

WebsocketTppConfig WebsocketTppConfig::create(WebsocketOptions options)
{
    if (!options._host.empty()) {
        auto uri = std::make_shared<websocketpp::uri>(options._host);
        if (uri->get_valid()) {
            return WebsocketTppConfig(std::move(uri), std::move(options));
        }
    }
    return {};
}

bool WebsocketTppConfig::secure() const noexcept
{
    if (const auto& u = uri()) {
        return u->get_secure();
    }
    return false;
}

} // namespace LiveKitCpp
#endif
