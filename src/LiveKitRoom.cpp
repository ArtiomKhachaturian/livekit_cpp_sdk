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

LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket::EndPoint> socket,
                         PeerConnectionFactory* pcf,
                         const SignalOptions& signalOptions)
    : _engine(std::make_unique<RTCEngine>(signalOptions, pcf, std::move(socket)))
{
}

LiveKitRoom::~LiveKitRoom()
{
}

bool LiveKitRoom::connect(std::string host, std::string authToken)
{
    // dummy
    /*_impl->_client.setHost(std::move(host));
    _impl->_client.setAuthToken(std::move(authToken));
    return _impl->_client.connect();*/
}

void LiveKitRoom::disconnect()
{
    //_impl->_client.disconnect();
}

#else
struct LiveKitRoom::Impl {};
    
LiveKitRoom::LiveKitRoom(std::unique_ptr<Websocket::EndPoint>,
                         PeerConnectionFactory*,
                         const ConnectOptions&,
                         const RoomOptions&) {}

LiveKitRoom::~LiveKitRoom() {}

bool LiveKitRoom::connect(std::string, std::string) { return false; }

void LiveKitRoom::disconnect() {}

#endif

} // namespace LiveKitCpp
