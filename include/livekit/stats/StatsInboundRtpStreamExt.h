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
#pragma once // StatsInboundRtpStreamExt.h
#include "StatsReceivedRtpStreamExt.h"

namespace LiveKitCpp
{

// https://w3c.github.io/webrtc-stats/#inboundrtpstats-dict*
class StatsInboundRtpStreamExt : public StatsReceivedRtpStreamExt
{
public:
    virtual std::optional<std::string> playoutId() const = 0;
    virtual std::optional<std::string> trackIdentifier() const = 0;
    virtual std::optional<std::string> mid() const = 0;
    virtual std::optional<std::string> remoteId() const = 0;
    virtual std::optional<uint32_t> packetsReceived() const = 0;
    virtual std::optional<uint64_t> packetsDiscarded() const = 0;
    virtual std::optional<uint64_t> fecPacketsReceived() const = 0;
    virtual std::optional<uint64_t> fecBytesReceived() const = 0;
    virtual std::optional<uint64_t> fecPacketsDiscarded() const = 0;
    // Inbound FEC SSRC. Only present if a mechanism like FlexFEC is negotiated.
    virtual std::optional<uint32_t> fecSsrc() const = 0;
    virtual std::optional<uint64_t> bytesReceived() const = 0;
    virtual std::optional<uint64_t> headerBytesReceived() const = 0;
    // Inbound RTX stats. Only defined when RTX is used and it is therefore
    // possible to distinguish retransmissions.
    virtual std::optional<uint64_t> retransmittedPacketsReceived() const = 0;
    virtual std::optional<uint64_t> retransmittedBytesReceived() const = 0;
    virtual std::optional<uint32_t> rtxSsrc() const = 0;
    virtual std::optional<double> lastPacketReceivedTimestamp() const = 0;
    virtual std::optional<double> jitterBufferDelay() const = 0;
    virtual std::optional<double> jitterBufferTargetDelay() const = 0;
    virtual std::optional<double> jitterBufferMinimumDelay() const = 0;
    virtual std::optional<uint64_t> jitterBufferEmittedCount() const = 0;
    virtual std::optional<uint64_t> totalSamplesReceived() const = 0;
    virtual std::optional<uint64_t> concealedSamples() const = 0;
    virtual std::optional<uint64_t> silentConcealedSamples() const = 0;
    virtual std::optional<uint64_t> concealmentEvents() const = 0;
    virtual std::optional<uint64_t> insertedSamplesForDeceleration() const = 0;
    virtual std::optional<uint64_t> removedSamplesForAcceleration() const = 0;
    virtual std::optional<double> audioLevel() const = 0;
    virtual std::optional<double> totalAudioEnergy() const = 0;
    virtual std::optional<double> totalSamplesDuration() const = 0;
    // Stats below are only implemented or defined for video.
    virtual std::optional<uint32_t> framesReceived() const = 0;
    virtual std::optional<uint32_t> frameWidth() const = 0;
    virtual std::optional<uint32_t> frameHeight() const = 0;
    virtual std::optional<double> framesPerSecond() const = 0;
    virtual std::optional<uint32_t> framesDecoded() const = 0;
    virtual std::optional<uint32_t> keyFramesDecoded() const = 0;
    virtual std::optional<uint32_t> framesDropped() const = 0;
    virtual std::optional<double> totalDecodeTime() const = 0;
    virtual std::optional<double> totalProcessingDelay() const = 0;
    virtual std::optional<double> totalAssemblyTime() const = 0;
    virtual std::optional<uint32_t> framesAssembledFromMultiplePackets() const = 0;
    virtual std::optional<double> totalInterFrameDelay() const = 0;
    virtual std::optional<double> totalSquaredInterFrameDelay() const = 0;
    virtual std::optional<uint32_t> pauseCount() const = 0;
    virtual std::optional<double> totalPausesDuration() const = 0;
    virtual std::optional<uint32_t> freezeCount() const = 0;
    virtual std::optional<double> totalFreezesDuration() const = 0;
    // https://w3c.github.io/webrtc-provisional-stats/#dom-rtcinboundrtpstreamstats-contenttype
    virtual std::optional<std::string> contentType() const = 0;
    // Only populated if audio/video sync is enabled.
    // TODO(https://crbug.com/webrtc/14177): Expose even if A/V sync is off?
    virtual std::optional<double> estimatedPlayoutTimestamp() const = 0;
    // Only defined for video.
    // In JavaScript, this is only exposed if HW exposure is allowed.
    virtual std::optional<std::string> decoderImplementation() const = 0;
    // FIR and PLI counts are only defined for |kind == "video"|.
    virtual std::optional<uint32_t> firCount() const = 0;
    virtual std::optional<uint32_t> pliCount() const = 0;
    virtual std::optional<uint32_t> nackCount() const = 0;
    virtual std::optional<uint64_t> qpSum() const = 0;
    virtual std::optional<double> totalCorruptionProbability() const = 0;
    virtual std::optional<double> totalSquaredCorruptionProbability() const = 0;
    virtual std::optional<uint64_t> corruptionMeasurements() const = 0;
    virtual std::optional<std::string> googTimingFrameInfo() const = 0;
    // In JavaScript, this is only exposed if HW exposure is allowed.
    virtual std::optional<bool> powerEfficientDecoder() const = 0;
    virtual std::optional<uint64_t> jitterBufferFlushes() const = 0;
    virtual std::optional<uint64_t> delayedPacketOutageSamples() const = 0;
    virtual std::optional<double> relativePacketArrivalDelay() const = 0;
    virtual std::optional<uint32_t> interruptionCount() const = 0;
    virtual std::optional<double> totalInterruptionDuration() const = 0;
    virtual std::optional<double> minPlayoutDelay() const = 0;
};

} // namespace LiveKitCpp
