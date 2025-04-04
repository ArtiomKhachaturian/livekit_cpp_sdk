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
#pragma once // StatsInboundRtpStreamImpl.h
#include "stats/StatsInboundRtpStreamExt.h"
#include "StatsReceivedRtpStreamImpl.h"

namespace LiveKitCpp
{

class StatsInboundRtpStreamImpl : public StatsReceivedRtpStreamImpl<webrtc::RTCInboundRtpStreamStats,
                                                                    StatsInboundRtpStreamExt>
{
    using Base = StatsReceivedRtpStreamImpl<webrtc::RTCInboundRtpStreamStats, StatsInboundRtpStreamExt>;
public:
    StatsInboundRtpStreamImpl(const webrtc::RTCInboundRtpStreamStats* stats,
                              const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::InboundRtp; }
    // impl. of StatsInboundRtpStreamExt
    std::optional<std::string> playoutId() const final;
    std::optional<std::string> trackIdentifier() const final;
    std::optional<std::string> mid() const final;
    std::optional<std::string> remoteId() const final;
    std::optional<uint32_t> packetsReceived() const final;
    std::optional<uint64_t> packetsDiscarded() const final;
    std::optional<uint64_t> fecPacketsReceived() const final;
    std::optional<uint64_t> fecBytesReceived() const final;
    std::optional<uint64_t> fecPacketsDiscarded() const final;
    std::optional<uint32_t> fecSsrc() const final;
    std::optional<uint64_t> bytesReceived() const final;
    std::optional<uint64_t> headerBytesReceived() const final;
    std::optional<uint64_t> retransmittedPacketsReceived() const final;
    std::optional<uint64_t> retransmittedBytesReceived() const final;
    std::optional<uint32_t> rtxSsrc() const final;
    std::optional<double> lastPacketReceivedTimestamp() const final;
    std::optional<double> jitterBufferDelay() const final;
    std::optional<double> jitterBufferTargetDelay() const final;
    std::optional<double> jitterBufferMinimumDelay() const final;
    std::optional<uint64_t> jitterBufferEmittedCount() const final;
    std::optional<uint64_t> totalSamplesReceived() const final;
    std::optional<uint64_t> concealedSamples() const final;
    std::optional<uint64_t> silentConcealedSamples() const final;
    std::optional<uint64_t> concealmentEvents() const final;
    std::optional<uint64_t> insertedSamplesForDeceleration() const final;
    std::optional<uint64_t> removedSamplesForAcceleration() const final;
    std::optional<double> audioLevel() const final;
    std::optional<double> totalAudioEnergy() const final;
    std::optional<double> totalSamplesDuration() const final;
    std::optional<uint32_t> framesReceived() const final;
    std::optional<uint32_t> frameWidth() const final;
    std::optional<uint32_t> frameHeight() const final;
    std::optional<double> framesPerSecond() const final;
    std::optional<uint32_t> framesDecoded() const final;
    std::optional<uint32_t> keyFramesDecoded() const final;
    std::optional<uint32_t> framesDropped() const final;
    std::optional<double> totalDecodeTime() const final;
    std::optional<double> totalProcessingDelay() const final;
    std::optional<double> totalAssemblyTime() const final;
    std::optional<uint32_t> framesAssembledFromMultiplePackets() const final;
    std::optional<double> totalInterFrameDelay() const final;
    std::optional<double> totalSquaredInterFrameDelay() const final;
    std::optional<uint32_t> pauseCount() const final;
    std::optional<double> totalPausesDuration() const final;
    std::optional<uint32_t> freezeCount() const final;
    std::optional<double> totalFreezesDuration() const final;
    std::optional<std::string> contentType() const final;
    std::optional<double> estimatedPlayoutTimestamp() const final;
    std::optional<std::string> decoderImplementation() const final;
    std::optional<uint32_t> firCount() const final;
    std::optional<uint32_t> pliCount() const final;
    std::optional<uint32_t> nackCount() const final;
    std::optional<uint64_t> qpSum() const final;
    std::optional<double> totalCorruptionProbability() const final;
    std::optional<double> totalSquaredCorruptionProbability() const final;
    std::optional<uint64_t> corruptionMeasurements() const final;
    std::optional<std::string> googTimingFrameInfo() const final;
    std::optional<bool> powerEfficientDecoder() const final;
    std::optional<uint64_t> jitterBufferFlushes() const final;
    std::optional<uint64_t> delayedPacketOutageSamples() const final;
    std::optional<double> relativePacketArrivalDelay() const final;
    std::optional<uint32_t> interruptionCount() const final;
    std::optional<double> totalInterruptionDuration() const final;
    std::optional<double> minPlayoutDelay() const final;
};

} // namespace LiveKitCpp
