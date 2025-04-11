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
#pragma once // StatsSentRtpStreamImpl.h
#include "livekit/rtc/stats/StatsSentRtpStreamExt.h"
#include "StatsRtpStreamImpl.h"

namespace LiveKitCpp
{

template <class TRtcStats, class TBaseInterface>
class StatsSentRtpStreamImpl : public StatsRtpStreamImpl<TRtcStats, TBaseInterface>
{
    static_assert(std::is_base_of_v<webrtc::RTCSentRtpStreamStats, TRtcStats>);
    static_assert(std::is_base_of_v<StatsSentRtpStreamExt, TBaseInterface>);
    using Base = StatsRtpStreamImpl<TRtcStats, TBaseInterface>;
public:
    StatsSentRtpStreamImpl(const TRtcStats* stats,
                           const std::shared_ptr<const StatsReportData>& data);
    // impl. of StatsSentRtpStreamExt
    std::optional<uint64_t> packetsSent() const final;
    std::optional<uint64_t> bytesSent() const final;
};

template <class TRtcStats, class TBaseInterface>
inline StatsSentRtpStreamImpl<TRtcStats, TBaseInterface>::
    StatsSentRtpStreamImpl(const TRtcStats* stats,
                           const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<uint64_t> StatsSentRtpStreamImpl<TRtcStats, TBaseInterface>::
    packetsSent() const
{
    if (Base::_stats) {
        return Base::_stats->packets_sent;
    }
    return {};
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<uint64_t> StatsSentRtpStreamImpl<TRtcStats, TBaseInterface>::
    bytesSent() const
{
    if (Base::_stats) {
        return Base::_stats->bytes_sent;
    }
    return {};
}

} // namespace LiveKitCpp
