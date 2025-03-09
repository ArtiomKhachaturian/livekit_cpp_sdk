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
#pragma once // RoomUtils.h
#include "rtc/ICEServer.h"
#include "rtc/SessionDescription.h"
#include "rtc/TrickleRequest.h"
#include <api/peer_connection_interface.h>
#include <memory>
#include <optional>
#include <vector>

namespace LiveKitCpp
{

enum class IceTransportPolicy;

class RoomUtils
{
public:
    static std::unique_ptr<webrtc::IceCandidateInterface> map(const TrickleRequest& trickle,
                                                              webrtc::SdpParseError* error = nullptr);
    static std::unique_ptr<webrtc::IceCandidateInterface> map(const std::string& candidateInit,
                                                              webrtc::SdpParseError* error = nullptr);
    static bool map(const webrtc::IceCandidateInterface* candidate, std::string& candidateInit);
    static std::optional<SessionDescription> map(const webrtc::SessionDescriptionInterface* desc);
    static std::unique_ptr<webrtc::SessionDescriptionInterface> map(const SessionDescription& desc,
                                                                    webrtc::SdpParseError* error = nullptr);
    static webrtc::PeerConnectionInterface::IceServer map(const ICEServer& server);
    static webrtc::PeerConnectionInterface::IceServers map(const std::vector<ICEServer>& servers);
    static webrtc::PeerConnectionInterface::IceTransportsType map(IceTransportPolicy policy);
};

inline std::string sdpTypeToString(webrtc::SdpType type) { // for logging
    return webrtc::SdpTypeToString(type);
}

} // namespace LiveKitCpp
