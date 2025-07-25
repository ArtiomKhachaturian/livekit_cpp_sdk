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
#pragma once // StatsVideoSourceImpl.h
#include "livekit/rtc/stats/StatsVideoSourceExt.h"
#include "StatsMediaSourceImpl.h"

namespace LiveKitCpp
{

class StatsVideoSourceImpl : public StatsMediaSourceImpl<webrtc::RTCVideoSourceStats,
                                                         StatsVideoSourceExt>
{
    using Base = StatsMediaSourceImpl<webrtc::RTCVideoSourceStats, StatsVideoSourceExt>;
public:
    StatsVideoSourceImpl(const webrtc::RTCVideoSourceStats* stats,
                         const std::shared_ptr<const StatsReportData>& data);
    // impl. of StatsVideoSourceExt
    std::optional<uint32_t> width() const final;
    std::optional<uint32_t> height() const final;
    std::optional<uint32_t> frames() const final;
    std::optional<double> framesPerSecond() const final;
};

} // namespace LiveKitCpp
