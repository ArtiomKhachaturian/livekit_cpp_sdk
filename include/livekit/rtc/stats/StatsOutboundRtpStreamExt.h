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
#pragma once // StatsOutboundRtpStreamExt.h
#include "livekit/stats/StatsSentRtpStreamExt.h"
#include <map>

namespace LiveKitCpp
{

// https://w3c.github.io/webrtc-stats/#outboundrtpstats-dict*
class StatsOutboundRtpStreamExt : public StatsSentRtpStreamExt
{
public:
    virtual std::optional<std::string> mediaSourceId() const = 0;
    virtual std::optional<std::string> remoteId() const = 0;
    virtual std::optional<std::string> mid() const = 0;
    virtual std::optional<std::string> rid() const = 0;
    virtual std::optional<uint64_t> retransmittedPacketsSent() const = 0;
    virtual std::optional<uint64_t> headerBytesSent() const = 0;
    virtual std::optional<uint64_t> retransmittedBytesSent() const = 0;
    virtual std::optional<double> targetBitrate() const = 0;
    virtual std::optional<uint32_t> framesEncoded() const = 0;
    virtual std::optional<uint32_t> keyFramesEncoded() const = 0;
    virtual std::optional<double> totalEncodeTime() const = 0;
    virtual std::optional<uint64_t> totalEncodedBytesTarget() const = 0;
    virtual std::optional<uint32_t> frameWidth() const = 0;
    virtual std::optional<uint32_t> frameHeight() const = 0;
    virtual std::optional<double> framesPerSecond() const = 0;
    virtual std::optional<uint32_t> framesSent() const = 0;
    virtual std::optional<uint32_t> hugeFramesSent() const = 0;
    virtual std::optional<double> totalPacketSendDelay() const = 0;
    virtual std::optional<std::string> qualityLimitationReason() const = 0;
    virtual std::optional<std::map<std::string, double>> qualityLimitationDurations() const = 0;
    // https://w3c.github.io/webrtc-stats/#dom-rtcoutboundrtpstreamstats-qualitylimitationresolutionchanges
    virtual std::optional<uint32_t> qualityLimitationResolutionChanges() const = 0;
    // https://w3c.github.io/webrtc-provisional-stats/#dom-rtcoutboundrtpstreamstats-contenttype
    virtual std::optional<std::string> contentType() const = 0;
    // In JavaScript, this is only exposed if HW exposure is allowed.
    // Only implemented for video.
    // TODO(https://crbug.com/webrtc/14178): Implement for audio as well.
    virtual std::optional<std::string> encoderImplementation() const = 0;
    // FIR and PLI counts are only defined for |kind == "video"|.
    virtual std::optional<uint32_t> firCount() const = 0;
    virtual std::optional<uint32_t> pliCount() const = 0;
    virtual std::optional<uint32_t> nackCount() const = 0;
    virtual std::optional<uint64_t> qpSum() const = 0;
    virtual std::optional<bool> active() const = 0;
    // In JavaScript, this is only exposed if HW exposure is allowed.
    virtual std::optional<bool> powerEfficientEncoder() const = 0;
    virtual std::optional<std::string> scalabilityMode() const = 0;
    // RTX ssrc. Only present if RTX is negotiated.
    virtual std::optional<uint32_t> rtxSsrc() const = 0;
};

} // namespace LiveKitCpp
