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
#pragma once // StatsIceCandidatePairExt.h
#include <optional>
#include <string>

namespace LiveKitCpp
{

// https://www.w3.org/TR/webrtc-stats/#dom-rtcicecandidatepairstats
class StatsIceCandidatePairExt
{
public:
    virtual ~StatsIceCandidatePairExt() = default;
    virtual std::optional<std::string> transportId() const = 0;
    virtual std::optional<std::string> localCandidateId() const = 0;
    virtual std::optional<std::string> remoteCandidateId() const = 0;
    virtual std::optional<std::string> state() const = 0;
    // Obsolete: priority
    virtual std::optional<uint64_t> priority() const = 0;
    virtual std::optional<bool> nominated() const = 0;
    // `writable` does not exist in the spec and old comments suggest it used to
    // exist but was incorrectly implemented.
    virtual std::optional<bool> writable() const = 0;
    virtual std::optional<uint64_t> packetsSent() const = 0;
    virtual std::optional<uint64_t> packetsReceived() const = 0;
    virtual std::optional<uint64_t> bytesSent() const = 0;
    virtual std::optional<uint64_t> bytesReceived() const = 0;
    virtual std::optional<double> totalRoundTripTime() const = 0;
    virtual std::optional<double> currentRoundTripTime() const = 0;
    virtual std::optional<double> availableOutgoingBitrate() const = 0;
    virtual std::optional<double> availableIncomingBitrate() const = 0;
    virtual std::optional<uint64_t> requestsReceived() const = 0;
    virtual std::optional<uint64_t> requestsSent() const = 0;
    virtual std::optional<uint64_t> responsesReceived() const = 0;
    virtual std::optional<uint64_t> responsesSent() const = 0;
    virtual std::optional<uint64_t> consentRequestsSent() const = 0;
    virtual std::optional<uint64_t> packetsDiscardedOnSend() const = 0;
    virtual std::optional<uint64_t> bytesDiscardedOnSend() const = 0;
    virtual std::optional<double> lastPacketReceivedTimestamp() const = 0;
    virtual std::optional<double> lastPacketSentTimestamp() const = 0;
};

} // namespace LiveKitCpp
