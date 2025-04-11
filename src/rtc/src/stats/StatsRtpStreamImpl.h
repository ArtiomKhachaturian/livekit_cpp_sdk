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
#pragma once // StatsRtpStreamImpl.h
#include "livekit/rtc/stats/StatsRtpStreamExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

template <class TRtcStats, class TBaseInterface>
class StatsRtpStreamImpl : public StatsDataImpl<TRtcStats, TBaseInterface>
{
    static_assert(std::is_base_of_v<webrtc::RTCRtpStreamStats, TRtcStats>);
    static_assert(std::is_base_of_v<StatsRtpStreamExt, TBaseInterface>);
    using Base = StatsDataImpl<TRtcStats, TBaseInterface>;
public:
    StatsRtpStreamImpl(const TRtcStats* stats,
                       const std::shared_ptr<const StatsReportData>& data);
    // impl. of StatsRtpStreamExt
    std::optional<uint32_t> ssrc() const final;
    std::optional<std::string> kind() const final;
    std::optional<std::string> transportId() const final;
    std::optional<std::string> codecId() const final;
};

template <class TRtcStats, class TBaseInterface>
inline StatsRtpStreamImpl<TRtcStats, TBaseInterface>::
    StatsRtpStreamImpl(const TRtcStats* stats, const std::shared_ptr<const StatsReportData>& data)
        : Base(stats, data)
{
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<uint32_t> StatsRtpStreamImpl<TRtcStats, TBaseInterface>::ssrc() const
{
    if (Base::_stats) {
        return Base::_stats->ssrc;
    }
    return {};
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<std::string> StatsRtpStreamImpl<TRtcStats, TBaseInterface>::kind() const
{
    if (Base::_stats) {
        return Base::_stats->kind;
    }
    return {};
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<std::string> StatsRtpStreamImpl<TRtcStats, TBaseInterface>::transportId() const
{
    if (Base::_stats) {
        return Base::_stats->transport_id;
    }
    return {};
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<std::string> StatsRtpStreamImpl<TRtcStats, TBaseInterface>::codecId() const
{
    if (Base::_stats) {
        return Base::_stats->codec_id;
    }
    return {};
}

} // namespace LiveKitCpp
