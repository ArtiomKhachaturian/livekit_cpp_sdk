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
#pragma once // StatsOutboundRtpStreamImpl.h
#include "stats/StatsOutboundRtpStreamExt.h"
#include "StatsSentRtpStreamImpl.h"

namespace LiveKitCpp
{

class StatsOutboundRtpStreamImpl : public StatsSentRtpStreamImpl<webrtc::RTCOutboundRtpStreamStats,
                                                                 StatsOutboundRtpStreamExt>
{
    using Base = StatsSentRtpStreamImpl<webrtc::RTCOutboundRtpStreamStats, StatsOutboundRtpStreamExt>;
public:
    StatsOutboundRtpStreamImpl(const webrtc::RTCOutboundRtpStreamStats* stats,
                               const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::OutboundRtp; }
    // impl. of StatsOutboundRtpStreamExt
    std::optional<std::string> mediaSourceId() const final;
    std::optional<std::string> remoteId() const final;
    std::optional<std::string> mid() const final;
    std::optional<std::string> rid() const final;
    std::optional<uint64_t> retransmittedPacketsSent() const final;
    std::optional<uint64_t> headerBytesSent() const final;
    std::optional<uint64_t> retransmittedBytesSent() const final;
    std::optional<double> targetBitrate() const final;
    std::optional<uint32_t> framesEncoded() const final;
    std::optional<uint32_t> keyFramesEncoded() const final;
    std::optional<double> totalEncodeTime() const final;
    std::optional<uint64_t> totalEncodedBytesTarget() const final;
    std::optional<uint32_t> frameWidth() const final;
    std::optional<uint32_t> frameHeight() const final;
    std::optional<double> framesPerSecond() const final;
    std::optional<uint32_t> framesSent() const final;
    std::optional<uint32_t> hugeFramesSent() const final;
    std::optional<double> totalPacketSendDelay() const final;
    std::optional<std::string> qualityLimitationReason() const final;
    std::optional<std::map<std::string, double>> qualityLimitationDurations() const final;
    std::optional<uint32_t> qualityLimitationResolutionChanges() const final;
    std::optional<std::string> contentType() const final;
    std::optional<std::string> encoderImplementation() const final;
    std::optional<uint32_t> firCount() const final;
    std::optional<uint32_t> pliCount() const final;
    std::optional<uint32_t> nackCount() const final;
    std::optional<uint64_t> qpSum() const final;
    std::optional<bool> active() const final;
    std::optional<bool> powerEfficientEncoder() const final;
    std::optional<std::string> scalabilityMode() const final;
    std::optional<uint32_t> rtxSsrc() const final;
};

} // namespace LiveKitCpp
