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
#include "Loggable.h"
#include "Websocket.h"
#include "ProtectedObjAliases.h"

namespace LiveKitCpp
{

class WebsocketTppServiceProvider;
class WebsocketTppApi;
struct WebsocketTls;
enum class State;

class WebsocketTpp : public LoggableShared<Websocket>
{
    template<class TClientType> class Impl;
    class TlsOn;
    class TlsOff;
    class Listener;
public:
    WebsocketTpp(std::shared_ptr<WebsocketTppServiceProvider> serviceProvider,
                 const std::shared_ptr<LogsReceiver>& logger = {});
    ~WebsocketTpp() final;
    // impl. of Websocket
    void addListener(WebsocketListener* listener) final;
    void removeListener(WebsocketListener* listener) final;
    bool open(WebsocketOptions options, uint64_t connectionId) final;
    void close() final;
    std::string host() const final;
    State state() const final;
    // impl. of CommandSender
    bool sendBinary(const std::shared_ptr<const MemoryBlock>& binary) final;
    bool sendText(const std::string_view& text) final;
private:
    std::shared_ptr<WebsocketTppApi> createImpl(WebsocketOptions options,
                                                uint64_t connectionId) const;
private:
    // increase read buffer size to optimize for huge audio/video messages:
    // 64 Kb instead of 16 by default, see details at
    // https://docs.websocketpp.org/structwebsocketpp_1_1config_1_1core.html#af1f28eec2b5e12b6d7cccb0c87835119
    static inline constexpr size_t _readBufferSize = 65536U;
    const std::shared_ptr<WebsocketTppServiceProvider> _serviceProvider;
    const std::shared_ptr<Listener> _listener;
    ProtectedSharedPtr<WebsocketTppApi> _impl;
};

} // namespace LiveKitCpp
#endif
