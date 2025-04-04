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
#pragma once // StatsRemoteInboundRtpStreamImpl.h
#include "stats/StatsRemoteInboundRtpStreamExt.h"
#include "StatsReceivedRtpStreamImpl.h"

namespace LiveKitCpp
{

class StatsRemoteInboundRtpStreamImpl : public StatsReceivedRtpStreamImpl<webrtc::RTCRemoteInboundRtpStreamStats,
                                                                         StatsRemoteInboundRtpStreamExt>
{
    using Base = StatsReceivedRtpStreamImpl<webrtc::RTCRemoteInboundRtpStreamStats, StatsRemoteInboundRtpStreamExt>;
public:
    StatsRemoteInboundRtpStreamImpl(const webrtc::RTCRemoteInboundRtpStreamStats* stats,
                                    const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::RemoteInboundRtp; }
    // impl. of StatsRemoteInboundRtpStreamExt
    std::optional<std::string> localId() const final;
    std::optional<double> roundTripTime() const final;
    std::optional<double> fractionLost() const final;
    std::optional<double> totalRoundTripTime() const final;
    std::optional<int32_t> roundTripTimeMeasurements() const final;
};

} // namespace LiveKitCpp
