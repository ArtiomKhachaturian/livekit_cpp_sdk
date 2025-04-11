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
#pragma once // StatsRemoteOutboundRtpStreamImpl.h
#ifdef WEBRTC_AVAILABLE
#include "stats/StatsRemoteOutboundRtpStreamExt.h"
#include "StatsSentRtpStreamImpl.h"

namespace LiveKitCpp
{

class StatsRemoteOutboundRtpStreamImpl : public StatsSentRtpStreamImpl<webrtc::RTCRemoteOutboundRtpStreamStats,
                                                                      StatsRemoteOutboundRtpStreamExt>
{
    using Base = StatsSentRtpStreamImpl<webrtc::RTCRemoteOutboundRtpStreamStats, StatsRemoteOutboundRtpStreamExt>;
public:
    StatsRemoteOutboundRtpStreamImpl(const webrtc::RTCRemoteOutboundRtpStreamStats* stats,
                                    const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::RemoteOutboundRtp; }
    // impl. of StatsRemoteOutboundRtpStreamExt
    std::optional<std::string> localId() const final;
    std::optional<double> remoteTimestamp() const final;
    std::optional<uint64_t> reportsSent() const final;
    std::optional<double> roundTripTime() const final;
    std::optional<uint64_t> roundTripTimeMeasurements() const final;
    std::optional<double> totalRoundTripTime() const final;
};

} // namespace LiveKitCpp
#endif
