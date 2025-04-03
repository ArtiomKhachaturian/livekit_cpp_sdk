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
#include "StatsIceCandidatePairImpl.h"
#ifdef WEBRTC_AVAILABLE

namespace LiveKitCpp
{

StatsIceCandidatePairImpl::StatsIceCandidatePairImpl(const webrtc::RTCIceCandidatePairStats* stats,
                                                     const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCIceCandidatePairStats, StatsIceCandidatePairExt>(stats, data)
{
}

std::optional<std::string> StatsIceCandidatePairImpl::transportId() const
{
    if (_stats) {
        return _stats->transport_id;
    }
    return {};
}

std::optional<std::string> StatsIceCandidatePairImpl::localCandidateId() const
{
    if (_stats) {
        return _stats->local_candidate_id;
    }
    return {};
}

std::optional<std::string> StatsIceCandidatePairImpl::remoteCandidateId() const
{
    if (_stats) {
        return _stats->remote_candidate_id;
    }
    return {};
}

std::optional<std::string> StatsIceCandidatePairImpl::state() const
{
    if (_stats) {
        return _stats->state;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::priority() const
{
    if (_stats) {
        return _stats->priority;
    }
    return {};
}

std::optional<bool> StatsIceCandidatePairImpl::nominated() const
{
    if (_stats) {
        return _stats->nominated;
    }
    return {};
}

std::optional<bool> StatsIceCandidatePairImpl::writable() const
{
    if (_stats) {
        return _stats->writable;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::packetsSent() const
{
    if (_stats) {
        return _stats->packets_sent;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::packetsReceived() const
{
    if (_stats) {
        return _stats->packets_received;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::bytesSent() const
{
    if (_stats) {
        return _stats->bytes_sent;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::bytesReceived() const
{
    if (_stats) {
        return _stats->bytes_received;
    }
    return {};
}

std::optional<double> StatsIceCandidatePairImpl::totalRoundTripTime() const
{
    if (_stats) {
        return _stats->total_round_trip_time;
    }
    return {};
}

std::optional<double> StatsIceCandidatePairImpl::currentRoundTripTime() const
{
    if (_stats) {
        return _stats->current_round_trip_time;
    }
    return {};
}

std::optional<double> StatsIceCandidatePairImpl::availableOutgoingBitrate() const
{
    if (_stats) {
        return _stats->available_outgoing_bitrate;
    }
    return {};
}

std::optional<double> StatsIceCandidatePairImpl::availableIncomingBitrate() const
{
    if (_stats) {
        return _stats->available_incoming_bitrate;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::requestsReceived() const
{
    if (_stats) {
        return _stats->requests_received;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::requestsSent() const
{
    if (_stats) {
        return _stats->requests_sent;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::responsesReceived() const
{
    if (_stats) {
        return _stats->requests_received;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::responsesSent() const
{
    if (_stats) {
        return _stats->responses_sent;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::consentRequestsSent() const
{
    if (_stats) {
        return _stats->consent_requests_sent;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::packetsDiscardedOnSend() const
{
    if (_stats) {
        return _stats->packets_discarded_on_send;
    }
    return {};
}

std::optional<uint64_t> StatsIceCandidatePairImpl::bytesDiscardedOnSend() const
{
    if (_stats) {
        return _stats->bytes_discarded_on_send;
    }
    return {};
}

std::optional<double> StatsIceCandidatePairImpl::lastPacketReceivedTimestamp() const
{
    if (_stats) {
        return _stats->last_packet_received_timestamp;
    }
    return {};
}

std::optional<double> StatsIceCandidatePairImpl::lastPacketSentTimestamp() const
{
    if (_stats) {
        return _stats->last_packet_sent_timestamp;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
