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
#pragma once // StatsMediaSourceImpl.h
#ifdef WEBRTC_AVAILABLE
#include "stats/StatsMediaSourceExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

template <class TRtcStats = webrtc::RTCMediaSourceStats, class TBaseInterface = StatsMediaSourceExt>
class StatsMediaSourceImpl : public StatsDataImpl<TRtcStats, TBaseInterface>
{
    static_assert(std::is_base_of_v<webrtc::RTCMediaSourceStats, TRtcStats>);
    static_assert(std::is_base_of_v<StatsMediaSourceExt, TBaseInterface>);
    using Base = StatsDataImpl<TRtcStats, TBaseInterface>;
public:
    StatsMediaSourceImpl(const TRtcStats* stats,
                         const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::MediaSource; }
    // impl. of StatsMediaSourceExt
    std::optional<std::string> trackIdentifier() const final;
    std::optional<std::string> kind() const final;
};

template <class TRtcStats, class TBaseInterface>
inline StatsMediaSourceImpl<TRtcStats, TBaseInterface>::
    StatsMediaSourceImpl(const TRtcStats* stats, const std::shared_ptr<const StatsReportData>& data)
        : Base(stats, data)
{
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<std::string> StatsMediaSourceImpl<TRtcStats, TBaseInterface>::
    trackIdentifier() const
{
    if (Base::_stats) {
        return Base::_stats->track_identifier;
    }
    return {};
}

template <class TRtcStats, class TBaseInterface>
inline std::optional<std::string> StatsMediaSourceImpl<TRtcStats, TBaseInterface>::
    kind() const
{
    if (Base::_stats) {
        return Base::_stats->kind;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
