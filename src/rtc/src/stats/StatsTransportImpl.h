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
#pragma once // StatsTransportImpl.h
#include "livekit/rtc/stats/StatsTransportExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsTransportImpl : public StatsDataImpl<webrtc::RTCTransportStats, StatsTransportExt>
{
public:
    StatsTransportImpl(const webrtc::RTCTransportStats* stats,
                       const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::Transport; }
    // impl. of StatsTransportExt
    std::optional<uint64_t> bytesSent() const final;
    std::optional<uint64_t> packetsSent() const final;
    std::optional<uint64_t> bytesReceived() const final;
    std::optional<uint64_t> packetsReceived() const final;
    std::optional<std::string> rtcpTransport_stats_id() const final;
    std::optional<std::string> dtlsState() const final;
    std::optional<std::string> selectedCandidatePairId() const final;
    std::optional<std::string> localCertificateId() const final;
    std::optional<std::string> remoteCertificateId() const final;
    std::optional<std::string> tlsVersion() const final;
    std::optional<std::string> dtlsCipher() const final;
    std::optional<std::string> dtlsRole() const final;
    std::optional<std::string> srtpCipher() const final;
    std::optional<uint32_t> selectedCandidatePairChanges() const final;
    std::optional<std::string> iceRole() const final;
    std::optional<std::string> iceLocalUsernameFragment() const final;
    std::optional<std::string> iceState() const final;
    
};

} // namespace LiveKitCpp
