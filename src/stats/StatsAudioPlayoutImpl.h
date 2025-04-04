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
#pragma once // StatsAudioPlayoutImpl.h
#include "stats/StatsAudioPlayoutExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsAudioPlayoutImpl : public StatsDataImpl<webrtc::RTCAudioPlayoutStats, StatsAudioPlayoutExt>
{
    using Base = StatsDataImpl<webrtc::RTCAudioPlayoutStats, StatsAudioPlayoutExt>;
public:
    StatsAudioPlayoutImpl(const webrtc::RTCAudioPlayoutStats* stats,
                          const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::MediaPlayout; }
    // impl. of StatsAudioPlayoutExt
    std::optional<std::string> kind() const final;
    std::optional<double> synthesizedSamplesDuration() const final;
    std::optional<uint64_t> synthesizedSamplesEvents() const final;
    std::optional<double> totalSamplesDuration() const final;
    std::optional<double> totalPlayoutDelay() const final;
    std::optional<uint64_t> totalSamplesCount() const final;
};

} // namespace LiveKitCpp
