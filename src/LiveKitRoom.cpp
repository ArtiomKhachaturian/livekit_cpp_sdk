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
#include "LiveKitRoomOptions.h"
#include "PeerConnectionFactory.h"
#include "websocket/WebsocketFactory.h"
#endif
#include "websocket/Websocket.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE
struct LiveKitRoom::Impl : public SignalServerListener,
                           public SignalTransportListener
{
    Impl(std::unique_ptr<Websocket> socket,
         PeerConnectionFactory* pcf,
         const LiveKitRoomOptions& options);
    ~Impl();
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    SignalClientWs _client;
};

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket> socket,
                         PeerConnectionFactory* pcf,
                         const LiveKitRoomOptions& options)
    : _impl(std::make_unique<Impl>(std::move(socket), pcf, options))
{
}

LiveKitRoom::~LiveKitRoom()
{
}

bool LiveKitRoom::connect(std::string host, std::string authToken)
{
    // dummy
    _impl->_client.setHost(std::move(host));
    _impl->_client.setAuthToken(std::move(authToken));
    return _impl->_client.connect();
}

void LiveKitRoom::disconnect()
{
    _impl->_client.disconnect();
}

LiveKitRoom::Impl::Impl(std::unique_ptr<Websocket> socket,
                        PeerConnectionFactory* pcf,
                        const LiveKitRoomOptions& options)
    : _pcf(pcf)
    , _client(std::move(socket), _pcf->logger().get())
{
    _client.setAdaptiveStream(options._adaptiveStream);
    _client.addServerListener(this);
    _client.addTransportListener(this);
}

LiveKitRoom::Impl::~Impl()
{
    _client.removeServerListener(this);
    _client.removeTransportListener(this);
}
#else
struct LiveKitRoom::Impl {};
    
LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket>, PeerConnectionFactory*,
                         const LiveKitRoomOptions&) {}

LiveKitRoom::~LiveKitRoom() {}

#endif

} // namespace LiveKitCpp
