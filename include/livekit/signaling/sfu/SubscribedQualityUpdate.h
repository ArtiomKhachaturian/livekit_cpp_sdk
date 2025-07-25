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
#pragma once // SubscribedQualityUpdate.h
#include "livekit/signaling/sfu/SubscribedCodec.h"
#include "livekit/signaling/sfu/SubscribedQuality.h"
#include <string>

namespace LiveKitCpp
{

// when max subscribe quality changed, used by dynamic broadcasting to disable unused layers
struct SubscribedQualityUpdate
{
    std::string _trackSid;
    std::vector<SubscribedQuality> _subscribedQualities;
    std::vector<SubscribedCodec> _subscribedCodecs;
};

} // namespace LiveKitCpp
