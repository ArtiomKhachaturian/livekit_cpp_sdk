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
#pragma once // WebsocketTppFactory.h
#ifdef USE_ZAPHOYD_TPP_SOCKETS
#include "Loggable.h"
#include "WebsocketFactory.h"

namespace LiveKitCpp
{

class WebsocketTppServiceProvider;

class WebsocketTppFactory : public SharedLoggerLoggable<WebsocketFactory>
{
    class ServiceProvider;
public:
    WebsocketTppFactory(const std::shared_ptr<LogsReceiver>& logger = {});
    // impl. of WebsocketFactory
    ~WebsocketTppFactory() override;
    std::unique_ptr<Websocket> create() const override;
private:
    std::shared_ptr<WebsocketTppServiceProvider> serviceProvider() const;
#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
private:
    const std::shared_ptr<ServiceProvider> _serviceProvider;
#endif
};

} // namespace LiveKitCpp
#endif
