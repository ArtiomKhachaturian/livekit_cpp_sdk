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
#pragma once // StatsAudioPlayoutExt.h
#include <optional>
#include <string>

namespace LiveKitCpp
{

// https://w3c.github.io/webrtc-stats/#playoutstats-dict*
class StatsAudioPlayoutExt
{
public:
    virtual ~StatsAudioPlayoutExt() = default;
    virtual std::optional<std::string> kind() const = 0;
    virtual std::optional<double> synthesizedSamplesDuration() const = 0;
    virtual std::optional<uint64_t> synthesizedSamplesEvents() const = 0;
    virtual std::optional<double> totalSamplesDuration() const = 0;
    virtual std::optional<double> totalPlayoutDelay() const = 0;
    virtual std::optional<uint64_t> totalSamplesCount() const = 0;
};

} // namespace LiveKitCpp
