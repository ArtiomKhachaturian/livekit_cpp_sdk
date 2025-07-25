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
#include "StatsAudioSourceImpl.h"

namespace LiveKitCpp
{

StatsAudioSourceImpl::StatsAudioSourceImpl(const webrtc::RTCAudioSourceStats* stats,
                                           const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

std::optional<double> StatsAudioSourceImpl::audioLevel() const
{
    if (_stats) {
        return _stats->audio_level;
    }
    return {};
}

std::optional<double> StatsAudioSourceImpl::totalAudioEnergy() const
{
    if (_stats) {
        return _stats->total_audio_energy;
    }
    return {};
}

std::optional<double> StatsAudioSourceImpl::totalSamplesDuration() const
{
    if (_stats) {
        return _stats->total_samples_duration;
    }
    return {};
}

std::optional<double> StatsAudioSourceImpl::echoReturnLoss() const
{
    if (_stats) {
        return _stats->echo_return_loss;
    }
    return {};
}

std::optional<double> StatsAudioSourceImpl::echoReturnLossEnhancement() const
{
    if (_stats) {
        return _stats->echo_return_loss_enhancement;
    }
    return {};
}

} // namespace LiveKitCpp
