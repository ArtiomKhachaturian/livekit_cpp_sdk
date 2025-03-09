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
#include "RoomUtils.h"
#include "rtc/ICETransportPolicy.h"
#include <nlohmann/json.hpp>

namespace LiveKitCpp
{

std::unique_ptr<webrtc::IceCandidateInterface> RoomUtils::map(const TrickleRequest& trickle,
                                                              webrtc::SdpParseError* error)
{
    return map(trickle._candidateInit, error);
}

std::unique_ptr<webrtc::IceCandidateInterface> RoomUtils::map(const std::string& candidateInit,
                                                              webrtc::SdpParseError* error)
{
    std::unique_ptr<webrtc::IceCandidateInterface> iceCandidate;
    if (!candidateInit.empty()) {
        try {
            auto json = nlohmann::json::parse(candidateInit);
            const auto candidate = json["candidate"].get<std::string>();
            const auto sdpMid = json["sdpMid"].get<std::string>();
            const auto sdpMLineIndex = json["sdpMLineIndex"].get<int>();
            iceCandidate.reset(webrtc::CreateIceCandidate(sdpMid, sdpMLineIndex, candidate, error));
        }
        catch (const std::exception& e) {
            if (error) {
                error->description = e.what();
            }
        }
    }
    return iceCandidate;
}

bool RoomUtils::map(const webrtc::IceCandidateInterface* candidate, std::string& candidateInit)
{
    if (candidate && candidate->ToString(&candidateInit)) {
        nlohmann::json iceCandidate;
        iceCandidate["iceCandidate"] = std::move(candidateInit);
        iceCandidate["sdpMid"] = candidate->sdp_mid();
        iceCandidate["sdpMLineIndex"] = candidate->sdp_mline_index();
        iceCandidate["usernameFragment"] = nullptr; // ??
        candidateInit = nlohmann::to_string(iceCandidate);
        return true;
    }
    return false;
}

std::optional<SessionDescription> RoomUtils::map(const webrtc::SessionDescriptionInterface* desc)
{
    if (desc) {
        SessionDescription sdp;
        if (desc->ToString(&sdp._sdp)) {
            sdp._type = desc->type();
            return sdp;
        }
    }
    return std::nullopt;
}

std::unique_ptr<webrtc::SessionDescriptionInterface> RoomUtils::
    map(const SessionDescription& desc, webrtc::SdpParseError* error)
{
    const auto type = webrtc::SdpTypeFromString(desc._type);
    if (type) {
        return webrtc::CreateSessionDescription(type.value(), desc._sdp, error);
    }
    if (error) {
        error->description = "incorrect descrpition type: " + desc._type;
    }
    return {};
}

webrtc::PeerConnectionInterface::IceServer RoomUtils::map(const ICEServer& server)
{
    webrtc::PeerConnectionInterface::IceServer webrtcIceSrv;
    webrtcIceSrv.username = server._username;
    webrtcIceSrv.password = server._credential;
    webrtcIceSrv.urls = server._urls;
    return webrtcIceSrv;
}

webrtc::PeerConnectionInterface::IceServers RoomUtils::map(const std::vector<ICEServer>& servers)
{
    webrtc::PeerConnectionInterface::IceServers webrtcIceSrvs;
    webrtcIceSrvs.reserve(servers.size());
    for (const auto& server : servers) {
        webrtcIceSrvs.push_back(map(server));
    }
    return webrtcIceSrvs;
}

webrtc::PeerConnectionInterface::IceTransportsType RoomUtils::map(IceTransportPolicy policy)
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
