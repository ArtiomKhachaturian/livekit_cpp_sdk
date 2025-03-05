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
#include "LiveKitRoom.h"
#ifdef WEBRTC_AVAILABLE
#include "SignalClientWs.h"
#include "SignalServerListener.h"
#include "SignalTransportListener.h"
#include "websocket/WebsocketFactory.h"
#endif
#include "websocket/Websocket.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE
struct LiveKitRoom::Impl : public SignalServerListener, public SignalTransportListener
{
    Impl(std::unique_ptr<Websocket> socket);
    ~Impl();
    static std::unique_ptr<LiveKitRoom::Impl> create(std::unique_ptr<Websocket> socket);
    const std::unique_ptr<SignalClientWs> _client;
};

LiveKitRoom::LiveKitRoom()
    : LiveKitRoom(WebsocketFactory::defaultFactory())
{
}

LiveKitRoom::LiveKitRoom(const WebsocketFactory& factory)
    : LiveKitRoom(factory.create())
{
}

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket> socket)
    : _impl(Impl::create(std::move(socket)))
{
}

LiveKitRoom::Impl::Impl(std::unique_ptr<Websocket> socket)
    : _client(std::make_unique<SignalClientWs>(std::move(socket)))
{
    _client->addServerListener(this);
    _client->addTransportListener(this);
}

LiveKitRoom::Impl::~Impl()
{
    _client->removeServerListener(this);
    _client->removeTransportListener(this);
}

std::unique_ptr<LiveKitRoom::Impl> LiveKitRoom::Impl::create(std::unique_ptr<Websocket> socket)
{
    if (socket) {
        return std::make_unique<Impl>(std::move(socket));
    }
    return {};
}
#else
struct LiveKitRoom::Impl {};
    
LiveKitRoom::LiveKitRoom() {}

LiveKitRoom::~LiveKitRoom() {}

LiveKitRoom::LiveKitRoom(const WebsocketFactory&) {}

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket>) {}
#endif

} // namespace LiveKitCpp
