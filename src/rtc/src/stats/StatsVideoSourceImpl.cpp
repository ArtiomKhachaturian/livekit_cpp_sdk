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
#include "StatsVideoSourceImpl.h"

namespace LiveKitCpp
{

StatsVideoSourceImpl::StatsVideoSourceImpl(const webrtc::RTCVideoSourceStats* stats,
                                           const std::shared_ptr<const StatsReportData>& data)
    : Base(stats, data)
{
}

std::optional<uint32_t> StatsVideoSourceImpl::width() const
{
    if (_stats) {
        return _stats->width;
    }
    return {};
}

std::optional<uint32_t> StatsVideoSourceImpl::height() const
{
    if (_stats) {
        return _stats->height;
    }
    return {};
}

std::optional<uint32_t> StatsVideoSourceImpl::frames() const
{
    if (_stats) {
        return _stats->frames;
    }
    return {};
}

std::optional<double> StatsVideoSourceImpl::framesPerSecond() const
{
    if (_stats) {
        return _stats->frames_per_second;
    }
    return {};
}

} // namespace LiveKitCpp
