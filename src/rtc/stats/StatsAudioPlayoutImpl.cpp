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
#include "StatsAudioPlayoutImpl.h"
#ifdef WEBRTC_AVAILABLE

namespace LiveKitCpp
{

StatsAudioPlayoutImpl::StatsAudioPlayoutImpl(const webrtc::RTCAudioPlayoutStats* stats,
                                             const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

std::optional<std::string> StatsAudioPlayoutImpl::kind() const
{
    if (_stats) {
        return _stats->kind;
    }
    return {};
}

std::optional<double> StatsAudioPlayoutImpl::synthesizedSamplesDuration() const
{
    if (_stats) {
        return _stats->synthesized_samples_duration;
    }
    return {};
}

std::optional<uint64_t> StatsAudioPlayoutImpl::synthesizedSamplesEvents() const
{
    if (_stats) {
        return _stats->synthesized_samples_events;
    }
    return {};
}

std::optional<double> StatsAudioPlayoutImpl::totalSamplesDuration() const
{
    if (_stats) {
        return _stats->total_samples_duration;
    }
    return {};
}

std::optional<double> StatsAudioPlayoutImpl::totalPlayoutDelay() const
{
    if (_stats) {
        return _stats->total_playout_delay;
    }
    return {};
}

std::optional<uint64_t> StatsAudioPlayoutImpl::totalSamplesCount() const
{
    if (_stats) {
        return _stats->total_samples_count;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
