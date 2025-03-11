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
#include "RTCEngine.h"
#include "PeerConnectionFactory.h"
#endif
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE

struct LiveKitRoom::Impl
{
    Impl(std::unique_ptr<Websocket::EndPoint> socket,
         PeerConnectionFactory* pcf,
         const Options& signalOptions,
         const std::shared_ptr<Bricks::Logger>& logger);
    RTCEngine _engine;
};

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket::EndPoint> socket,
                         PeerConnectionFactory* pcf,
                         const Options& signalOptions,
                         const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(std::make_unique<Impl>(std::move(socket), pcf, signalOptions, logger))
{
}

LiveKitRoom::~LiveKitRoom()
{
}

bool LiveKitRoom::connect(std::string host, std::string authToken)
{
    return _impl->_engine.connect(std::move(host), std::move(authToken));
}

void LiveKitRoom::disconnect()
{
    //_impl->_client.disconnect();
}

void LiveKitRoom::setMicrophoneEnabled(bool enable)
{
    _impl->_engine.unmuteMicrophone(enable);
}

LiveKitRoom::Impl::Impl(std::unique_ptr<Websocket::EndPoint> socket,
                        PeerConnectionFactory* pcf,
                        const Options& signalOptions,
                        const std::shared_ptr<Bricks::Logger>& logger)
    : _engine(signalOptions, pcf, std::move(socket), logger)
{
}

#else
struct LiveKitRoom::Impl {};
    
LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket::EndPoint>,
                         PeerConnectionFactory*,
                         const Options&,
                         const std::shared_ptr<Bricks::Logger>&) {}

LiveKitRoom::~LiveKitRoom() {}

bool LiveKitRoom::connect(std::string, std::string) { return false; }

void LiveKitRoom::disconnect() {}

void LiveKitRoom::setMicrophoneEnabled(bool) {}
#endif

} // namespace LiveKitCpp
