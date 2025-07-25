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
#pragma once // TrackInfoSeq.h
#include "livekit/signaling/sfu/TrackInfo.h"
#include <vector>

namespace LiveKitCpp
{

void findDifference(const std::vector<TrackInfo>& currentTracksInfo,
                    const std::vector<TrackInfo>& newTracksInfo,
                    std::vector<TrackInfo>* added = nullptr,
                    std::vector<TrackInfo>* removed = nullptr,
                    std::vector<TrackInfo>* updated = nullptr);

} // namespace LiveKitCpp
