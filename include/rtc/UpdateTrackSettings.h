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
#pragma once // UpdateTrackSettings.h
#include "rtc/VideoQuality.h"
#include <string>
#include <vector>

namespace LiveKitCpp
{

// Update settings of subscribed tracks
struct UpdateTrackSettings
{
    std::vector<std::string> _trackSids;
    // when true, the track is placed in a paused state, with no new data returned
    bool _disabled = {};
    // deprecated in favor of width & height
    VideoQuality _quality = {};
    // for video, width to receive
    uint32_t _width = {};
    // for video, height to receive
    uint32_t _height = {};
    uint32_t _fps = {};
    // subscription priority. 1 being the highest (0 is unset)
    // when unset, server sill assign priority based on the order of subscription
    // server will use priority in the following ways:
    // 1. when subscribed tracks exceed per-participant subscription limit, server will
    //    pause the lowest priority tracks
    // 2. when the network is congested, server will assign available bandwidth to
    //    higher priority tracks first. lowest priority tracks can be paused
    uint32_t _priority = {};
};

} // namespace LiveKitCpp
