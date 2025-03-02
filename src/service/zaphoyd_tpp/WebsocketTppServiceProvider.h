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
#include "WebsocketTppTypeDefs.h"

namespace LiveKitCpp
{

struct WebsocketTls;

class WebsocketTppServiceProvider
{
public:
    virtual ~WebsocketTppServiceProvider() = default;
    virtual void startService() = 0;
    virtual void stopService() = 0;
    virtual WebsocketTppIOSrv* service() = 0;
    virtual std::shared_ptr<WebsocketTppSSLCtx> createSSLContext(const WebsocketTls& tls) const = 0;
};

} // namespace LiveKitCpp
#endif
