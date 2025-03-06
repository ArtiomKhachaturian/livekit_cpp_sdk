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
#include "LiveKitRoomUtils.h"
#include "rtc/ICETransportPolicy.h"

namespace LiveKitCpp
{

webrtc::PeerConnectionInterface::IceServer LiveKitRoomUtils::map(const ICEServer& server)
{
    webrtc::PeerConnectionInterface::IceServer webrtcIceSrv;
    webrtcIceSrv.username = server._username;
    webrtcIceSrv.password = server._credential;
    webrtcIceSrv.urls = server._urls;
    return webrtcIceSrv;
}

webrtc::PeerConnectionInterface::IceServers LiveKitRoomUtils::map(const std::vector<ICEServer>& servers)
{
    webrtc::PeerConnectionInterface::IceServers webrtcIceSrvs;
    webrtcIceSrvs.reserve(servers.size());
    for (const auto& server : servers) {
        webrtcIceSrvs.push_back(map(server));
    }
    return webrtcIceSrvs;
}

webrtc::PeerConnectionInterface::IceTransportsType LiveKitRoomUtils::map(IceTransportPolicy policy)
{
    switch (policy) {
        case IceTransportPolicy::None:
            break;
        case IceTransportPolicy::Relay:
            return webrtc::PeerConnectionInterface::IceTransportsType::kRelay;
            break;
        case IceTransportPolicy::NoHost:
            return webrtc::PeerConnectionInterface::IceTransportsType::kNoHost;
        case IceTransportPolicy::All:
            return webrtc::PeerConnectionInterface::IceTransportsType::kAll;
    }
    return webrtc::PeerConnectionInterface::IceTransportsType::kNone;
}

} // namespace LiveKitCpp
