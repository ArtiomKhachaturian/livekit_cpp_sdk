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
#pragma once // SubscriptionResponse.h
#include "livekit/signaling/sfu/SubscriptionError.h"
#include <string>

namespace LiveKitCpp
{

// Subscription response, client should not expect any media from this subscription if it fails
struct SubscriptionResponse
{
    std::string _trackSid;
    SubscriptionError _err = {};
};

} // namespace LiveKitCpp
