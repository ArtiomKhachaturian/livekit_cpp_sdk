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
#pragma once // StatsIceCandidateImpl.h
#ifdef WEBRTC_AVAILABLE
#include "stats/StatsIceCandidateExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsIceCandidateImpl : public StatsDataImpl<webrtc::RTCIceCandidateStats, StatsIceCandidateExt>
{
public:
    StatsIceCandidateImpl(const webrtc::RTCIceCandidateStats* stats,
                          const std::shared_ptr<const StatsReportData>& data);
    // impl. of StatsIceCandidateExt
    std::optional<std::string> transportId() const final;
    std::optional<std::string> networkType() const final;
    std::optional<std::string> ip() const final;
    std::optional<std::string> address() const final;
    std::optional<int32_t> port() const final;
    std::optional<std::string> protocol() const final;
    std::optional<std::string> relayProtocol() const final;
    std::optional<std::string> candidateType() const final;
    std::optional<int32_t> priority() const final;
    std::optional<std::string> url() const final;
    std::optional<std::string> foundation() const final;
    std::optional<std::string> relatedAddress() const final;
    std::optional<int32_t> relatedPort() const final;
    std::optional<std::string> usernameFragment() const final;
    std::optional<std::string> tcpType() const final;
    std::optional<bool> vpn() const final;
    std::optional<std::string> networkAdapterType() const final;
};

} // namespace LiveKitCpp
#endif
