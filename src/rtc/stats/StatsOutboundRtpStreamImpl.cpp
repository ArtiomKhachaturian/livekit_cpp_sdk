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
#include "StatsOutboundRtpStreamImpl.h"
#ifdef WEBRTC_AVAILABLE

namespace LiveKitCpp
{

StatsOutboundRtpStreamImpl::StatsOutboundRtpStreamImpl(const webrtc::RTCOutboundRtpStreamStats* stats,
                                                       const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

std::optional<std::string> StatsOutboundRtpStreamImpl::mediaSourceId() const
{
    if (_stats) {
        return _stats->media_source_id;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::remoteId() const
{
    if (_stats) {
        return _stats->remote_id;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::mid() const
{
    if (_stats) {
        return _stats->mid;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::rid() const
{
    if (_stats) {
        return _stats->rid;
    }
    return {};
}

std::optional<uint64_t> StatsOutboundRtpStreamImpl::retransmittedPacketsSent() const
{
    if (_stats) {
        return _stats->retransmitted_packets_sent;
    }
    return {};
}

std::optional<uint64_t> StatsOutboundRtpStreamImpl::headerBytesSent() const
{
    if (_stats) {
        return _stats->header_bytes_sent;
    }
    return {};
}

std::optional<uint64_t> StatsOutboundRtpStreamImpl::retransmittedBytesSent() const
{
    if (_stats) {
        return _stats->retransmitted_bytes_sent;
    }
    return {};
}

std::optional<double> StatsOutboundRtpStreamImpl::targetBitrate() const
{
    if (_stats) {
        return _stats->target_bitrate;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::framesEncoded() const
{
    if (_stats) {
        return _stats->frames_encoded;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::keyFramesEncoded() const
{
    if (_stats) {
        return _stats->key_frames_encoded;
    }
    return {};
}

std::optional<double> StatsOutboundRtpStreamImpl::totalEncodeTime() const
{
    if (_stats) {
        return _stats->total_encode_time;
    }
    return {};
}

std::optional<uint64_t> StatsOutboundRtpStreamImpl::totalEncodedBytesTarget() const
{
    if (_stats) {
        return _stats->total_encoded_bytes_target;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::frameWidth() const
{
    if (_stats) {
        return _stats->frame_width;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::frameHeight() const
{
    if (_stats) {
        return _stats->frame_height;
    }
    return {};
}

std::optional<double> StatsOutboundRtpStreamImpl::framesPerSecond() const
{
    if (_stats) {
        return _stats->frames_per_second;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::framesSent() const
{
    if (_stats) {
        return _stats->frames_sent;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::hugeFramesSent() const
{
    if (_stats) {
        return _stats->huge_frames_sent;
    }
    return {};
}

std::optional<double> StatsOutboundRtpStreamImpl::totalPacketSendDelay() const
{
    if (_stats) {
        return _stats->total_packet_send_delay;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::qualityLimitationReason() const
{
    if (_stats) {
        return _stats->quality_limitation_reason;
    }
    return {};
}

std::optional<std::map<std::string, double>> StatsOutboundRtpStreamImpl::
    qualityLimitationDurations() const
{
    if (_stats) {
        return _stats->quality_limitation_durations;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::qualityLimitationResolutionChanges() const
{
    if (_stats) {
        return _stats->quality_limitation_resolution_changes;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::contentType() const
{
    if (_stats) {
        return _stats->content_type;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::encoderImplementation() const
{
    if (_stats) {
        return _stats->encoder_implementation;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::firCount() const
{
    if (_stats) {
        return _stats->fir_count;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::pliCount() const
{
    if (_stats) {
        return _stats->pli_count;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::nackCount() const
{
    if (_stats) {
        return _stats->nack_count;
    }
    return {};
}

std::optional<uint64_t> StatsOutboundRtpStreamImpl::qpSum() const
{
    if (_stats) {
        return _stats->qp_sum;
    }
    return {};
}

std::optional<bool> StatsOutboundRtpStreamImpl::active() const
{
    if (_stats) {
        return _stats->active;
    }
    return {};
}

std::optional<bool> StatsOutboundRtpStreamImpl::powerEfficientEncoder() const
{
    if (_stats) {
        return _stats->power_efficient_encoder;
    }
    return {};
}

std::optional<std::string> StatsOutboundRtpStreamImpl::scalabilityMode() const
{
    if (_stats) {
        return _stats->scalability_mode;
    }
    return {};
}

std::optional<uint32_t> StatsOutboundRtpStreamImpl::rtxSsrc() const
{
    if (_stats) {
        return _stats->rtx_ssrc;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
