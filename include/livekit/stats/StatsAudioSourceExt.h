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
#pragma once // StatsAudioSourceExt.h
#include "livekit/stats/StatsMediaSourceExt.h"

namespace LiveKitCpp
{

// https://w3c.github.io/webrtc-stats/#dom-rtcaudiosourcestats
class StatsAudioSourceExt : public StatsMediaSourceExt
{
public:
    virtual std::optional<double> audioLevel() const = 0;
    virtual std::optional<double> totalAudioEnergy() const = 0;
    virtual std::optional<double> totalSamplesDuration() const = 0;
    virtual std::optional<double> echoReturnLoss() const = 0;
    virtual std::optional<double> echoReturnLossEnhancement() const = 0;
};

} // namespace LiveKitCpp
