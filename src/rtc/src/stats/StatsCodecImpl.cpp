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
#include "StatsCodecImpl.h"

namespace LiveKitCpp
{

StatsCodecImpl::StatsCodecImpl(const webrtc::RTCCodecStats* stats,
                               const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCCodecStats, StatsCodecExt>(stats, data)
{
}

std::optional<std::string> StatsCodecImpl::transportId() const
{
    if (_stats) {
        return _stats->transport_id;
    }
    return {};
}

std::optional<uint32_t> StatsCodecImpl::payloadType() const
{
    if (_stats) {
        return _stats->payload_type;
    }
    return {};
}

std::optional<std::string> StatsCodecImpl::mimeType() const
{
    if (_stats) {
        return _stats->mime_type;
    }
    return {};
}

std::optional<uint32_t> StatsCodecImpl::clockRate() const
{
    if (_stats) {
        return _stats->clock_rate;
    }
    return {};
}

std::optional<uint32_t> StatsCodecImpl::channels() const
{
    if (_stats) {
        return _stats->channels;
    }
    return {};
}

std::optional<std::string> StatsCodecImpl::sdpFmtpLine() const
{
    if (_stats) {
        return _stats->sdp_fmtp_line;
    }
    return {};
}

} // namespace LiveKitCpp
