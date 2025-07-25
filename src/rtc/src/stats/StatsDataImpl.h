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
#pragma once // StatsDataImpl.h
#include "StatsData.h"
#include "StatsReportData.h"
#include <api/stats/rtcstats_objects.h>
#include <type_traits>

namespace LiveKitCpp
{

template <class TRtcStats, class... TBaseInterface>
class StatsDataImpl : public StatsData, public TBaseInterface...
{
    static_assert(std::is_base_of_v<webrtc::RTCStats, TRtcStats>);
public:
    StatsDataImpl(const TRtcStats* stats,
                  const std::shared_ptr<const StatsReportData>& data);
    ~StatsDataImpl() override = default;
    // final
    const webrtc::RTCStats* rtcStats() const final { return _stats; }
protected:
    const TRtcStats* const _stats;
private:
    const std::shared_ptr<const StatsReportData> _data;
};

template <class TRtcStats, class... TBaseInterface>
inline StatsDataImpl<TRtcStats, TBaseInterface...>::
    StatsDataImpl(const TRtcStats* stats,
                  const std::shared_ptr<const StatsReportData>& data)
    : _stats(stats)
    , _data(data)
{
}

} // namespace LiveKitCpp
