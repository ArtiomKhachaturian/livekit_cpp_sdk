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
#include "StatsInboundRtpStreamImpl.h"
#ifdef WEBRTC_AVAILABLE

namespace LiveKitCpp
{

StatsInboundRtpStreamImpl::StatsInboundRtpStreamImpl(const webrtc::RTCInboundRtpStreamStats* stats,
                                                     const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

std::optional<std::string> StatsInboundRtpStreamImpl::playoutId() const
{
    if (_stats) {
        return _stats->playout_id;
    }
    return {};
}

std::optional<std::string> StatsInboundRtpStreamImpl::trackIdentifier() const
{
    if (_stats) {
        return _stats->track_identifier;
    }
    return {};
}

std::optional<std::string> StatsInboundRtpStreamImpl::mid() const
{
    if (_stats) {
        return _stats->mid;
    }
    return {};
}

std::optional<std::string> StatsInboundRtpStreamImpl::remoteId() const
{
    if (_stats) {
        return _stats->remote_id;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::packetsReceived() const
{
    if (_stats) {
        return _stats->packets_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::packetsDiscarded() const
{
    if (_stats) {
        return _stats->packets_discarded;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::fecPacketsReceived() const
{
    if (_stats) {
        return _stats->fec_packets_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::fecBytesReceived() const
{
    if (_stats) {
        return _stats->fec_bytes_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::fecPacketsDiscarded() const
{
    if (_stats) {
        return _stats->fec_packets_discarded;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::fecSsrc() const
{
    if (_stats) {
        return _stats->fec_ssrc;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::bytesReceived() const
{
    if (_stats) {
        return _stats->bytes_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::headerBytesReceived() const
{
    if (_stats) {
        return _stats->header_bytes_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::retransmittedPacketsReceived() const
{
    if (_stats) {
        return _stats->retransmitted_packets_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::retransmittedBytesReceived() const
{
    if (_stats) {
        return _stats->retransmitted_bytes_received;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::rtxSsrc() const
{
    if (_stats) {
        return _stats->rtx_ssrc;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::lastPacketReceivedTimestamp() const
{
    if (_stats) {
        return _stats->last_packet_received_timestamp;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::jitterBufferDelay() const
{
    if (_stats) {
        return _stats->jitter_buffer_delay;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::jitterBufferTargetDelay() const
{
    if (_stats) {
        return _stats->jitter_buffer_target_delay;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::jitterBufferMinimumDelay() const
{
    if (_stats) {
        return _stats->jitter_buffer_minimum_delay;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::jitterBufferEmittedCount() const
{
    if (_stats) {
        return _stats->jitter_buffer_emitted_count;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::totalSamplesReceived() const
{
    if (_stats) {
        return _stats->total_samples_received;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::concealedSamples() const
{
    if (_stats) {
        return _stats->concealed_samples;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::silentConcealedSamples() const
{
    if (_stats) {
        return _stats->silent_concealed_samples;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::concealmentEvents() const
{
    if (_stats) {
        return _stats->concealment_events;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::insertedSamplesForDeceleration() const
{
    if (_stats) {
        return _stats->inserted_samples_for_deceleration;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::removedSamplesForAcceleration() const
{
    if (_stats) {
        return _stats->removed_samples_for_acceleration;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::audioLevel() const
{
    if (_stats) {
        return _stats->audio_level;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalAudioEnergy() const
{
    if (_stats) {
        return _stats->total_audio_energy;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalSamplesDuration() const
{
    if (_stats) {
        return _stats->total_samples_duration;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::framesReceived() const
{
    if (_stats) {
        return _stats->frames_received;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::frameWidth() const
{
    if (_stats) {
        return _stats->frame_width;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::frameHeight() const
{
    if (_stats) {
        return _stats->frame_height;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::framesPerSecond() const
{
    if (_stats) {
        return _stats->frames_per_second;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::framesDecoded() const
{
    if (_stats) {
        return _stats->frames_decoded;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::keyFramesDecoded() const
{
    if (_stats) {
        return _stats->key_frames_decoded;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::framesDropped() const
{
    if (_stats) {
        return _stats->frames_dropped;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalDecodeTime() const
{
    if (_stats) {
        return _stats->total_decode_time;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalProcessingDelay() const
{
    if (_stats) {
        return _stats->total_processing_delay;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalAssemblyTime() const
{
    if (_stats) {
        return _stats->total_assembly_time;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::framesAssembledFromMultiplePackets() const
{
    if (_stats) {
        return _stats->frames_assembled_from_multiple_packets;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalInterFrameDelay() const
{
    if (_stats) {
        return _stats->total_inter_frame_delay;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalSquaredInterFrameDelay() const
{
    if (_stats) {
        return _stats->total_squared_inter_frame_delay;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::pauseCount() const
{
    if (_stats) {
        return _stats->pause_count;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalPausesDuration() const
{
    if (_stats) {
        return _stats->total_pauses_duration;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::freezeCount() const
{
    if (_stats) {
        return _stats->freeze_count;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalFreezesDuration() const
{
    if (_stats) {
        return _stats->total_freezes_duration;
    }
    return {};
}

std::optional<std::string> StatsInboundRtpStreamImpl::contentType() const
{
    if (_stats) {
        return _stats->content_type;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::estimatedPlayoutTimestamp() const
{
    if (_stats) {
        return _stats->estimated_playout_timestamp;
    }
    return {};
}

std::optional<std::string> StatsInboundRtpStreamImpl::decoderImplementation() const
{
    if (_stats) {
        return _stats->decoder_implementation;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::firCount() const
{
    if (_stats) {
        return _stats->fir_count;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::pliCount() const
{
    if (_stats) {
        return _stats->pli_count;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::nackCount() const
{
    if (_stats) {
        return _stats->nack_count;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::qpSum() const
{
    if (_stats) {
        return _stats->qp_sum;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalCorruptionProbability() const
{
    if (_stats) {
        return _stats->total_corruption_probability;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalSquaredCorruptionProbability() const
{
    if (_stats) {
        return _stats->total_squared_corruption_probability;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::corruptionMeasurements() const
{
    if (_stats) {
        return _stats->corruption_measurements;
    }
    return {};
}

std::optional<std::string> StatsInboundRtpStreamImpl::googTimingFrameInfo() const
{
    if (_stats) {
        return _stats->goog_timing_frame_info;
    }
    return {};
}

std::optional<bool> StatsInboundRtpStreamImpl::powerEfficientDecoder() const
{
    if (_stats) {
        return _stats->power_efficient_decoder;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::jitterBufferFlushes() const
{
    if (_stats) {
        return _stats->jitter_buffer_flushes;
    }
    return {};
}

std::optional<uint64_t> StatsInboundRtpStreamImpl::delayedPacketOutageSamples() const
{
    if (_stats) {
        return _stats->delayed_packet_outage_samples;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::relativePacketArrivalDelay() const
{
    if (_stats) {
        return _stats->relative_packet_arrival_delay;
    }
    return {};
}

std::optional<uint32_t> StatsInboundRtpStreamImpl::interruptionCount() const
{
    if (_stats) {
        return _stats->interruption_count;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::totalInterruptionDuration() const
{
    if (_stats) {
        return _stats->total_interruption_duration;
    }
    return {};
}

std::optional<double> StatsInboundRtpStreamImpl::minPlayoutDelay() const
{
    if (_stats) {
        return _stats->min_playout_delay;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
