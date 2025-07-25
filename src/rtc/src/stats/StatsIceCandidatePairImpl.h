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
#pragma once // StatsIceCandidatePairImpl.h
#include "livekit/rtc/stats/StatsIceCandidatePairExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsIceCandidatePairImpl : public StatsDataImpl<webrtc::RTCIceCandidatePairStats, StatsIceCandidatePairExt>
{
public:
    StatsIceCandidatePairImpl(const webrtc::RTCIceCandidatePairStats* stats,
                              const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::CandidatePair; }
    // impl. of StatsIceCandidatePairExt
    std::optional<std::string> transportId() const final;
    std::optional<std::string> localCandidateId() const final;
    std::optional<std::string> remoteCandidateId() const final;
    std::optional<std::string> state() const final;
    std::optional<uint64_t> priority() const final;
    std::optional<bool> nominated() const final;
    std::optional<bool> writable() const final;
    std::optional<uint64_t> packetsSent() const final;
    std::optional<uint64_t> packetsReceived() const final;
    std::optional<uint64_t> bytesSent() const final;
    std::optional<uint64_t> bytesReceived() const final;
    std::optional<double> totalRoundTripTime() const final;
    std::optional<double> currentRoundTripTime() const final;
    std::optional<double> availableOutgoingBitrate() const final;
    std::optional<double> availableIncomingBitrate() const final;
    std::optional<uint64_t> requestsReceived() const final;
    std::optional<uint64_t> requestsSent() const final;
    std::optional<uint64_t> responsesReceived() const final;
    std::optional<uint64_t> responsesSent() const final;
    std::optional<uint64_t> consentRequestsSent() const final;
    std::optional<uint64_t> packetsDiscardedOnSend() const final;
    std::optional<uint64_t> bytesDiscardedOnSend() const final;
    std::optional<double> lastPacketReceivedTimestamp() const final;
    std::optional<double> lastPacketSentTimestamp() const final;
};

} // namespace LiveKitCpp
