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
#pragma once // StatsReceivedRtpStreamImpl.h
#ifdef WEBRTC_AVAILABLE
#include "stats/StatsReceivedRtpStreamExt.h"
#include "StatsRtpStreamImpl.h"

namespace LiveKitCpp
{

template<class TRtcStats, class TBaseInterface>
class StatsReceivedRtpStreamImpl : public StatsRtpStreamImpl<TRtcStats, TBaseInterface>
{
    static_assert(std::is_base_of_v<webrtc::RTCReceivedRtpStreamStats, TRtcStats>);
    static_assert(std::is_base_of_v<StatsReceivedRtpStreamExt, TBaseInterface>);
    using Base = StatsRtpStreamImpl<TRtcStats, TBaseInterface>;
public:
    StatsReceivedRtpStreamImpl(const TRtcStats* stats,
                               const std::shared_ptr<const StatsReportData>& data);
    // impl. of StatsReceivedRtpStreamExt
    std::optional<double> jitter() const final;
    std::optional<int32_t> packetsLost() const final;
};

template<class TRtcStats, class TBaseInterface>
inline StatsReceivedRtpStreamImpl<TRtcStats, TBaseInterface>::
    StatsReceivedRtpStreamImpl(const TRtcStats* stats,
                               const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

template<class TRtcStats, class TBaseInterface>
inline std::optional<double> StatsReceivedRtpStreamImpl<TRtcStats, TBaseInterface>::
    jitter() const
{
    if (Base::_stats) {
        return Base::_stats->jitter;
    }
    return {};
}

template<class TRtcStats, class TBaseInterface>
inline std::optional<int32_t> StatsReceivedRtpStreamImpl<TRtcStats, TBaseInterface>::
    packetsLost() const
{
    if (Base::_stats) {
        return Base::_stats->packets_lost;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
