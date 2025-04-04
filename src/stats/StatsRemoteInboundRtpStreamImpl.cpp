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
#include "StatsRemoteInboundRtpStreamImpl.h"

namespace LiveKitCpp
{

StatsRemoteInboundRtpStreamImpl::StatsRemoteInboundRtpStreamImpl(const webrtc::RTCRemoteInboundRtpStreamStats* stats,
                                                                 const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

std::optional<std::string> StatsRemoteInboundRtpStreamImpl::localId() const
{
    if (_stats) {
        return _stats->local_id;
    }
    return {};
}

std::optional<double> StatsRemoteInboundRtpStreamImpl::roundTripTime() const
{
    if (_stats) {
        return _stats->round_trip_time;
    }
    return {};
}

std::optional<double> StatsRemoteInboundRtpStreamImpl::fractionLost() const
{
    if (_stats) {
        return _stats->fraction_lost;
    }
    return {};
}

std::optional<double> StatsRemoteInboundRtpStreamImpl::totalRoundTripTime() const
{
    if (_stats) {
        return _stats->total_round_trip_time;
    }
    return {};
}

std::optional<int32_t> StatsRemoteInboundRtpStreamImpl::roundTripTimeMeasurements() const
{
    if (_stats) {
        return _stats->round_trip_time_measurements;
    }
    return {};
}

} // namespace LiveKitCpp
