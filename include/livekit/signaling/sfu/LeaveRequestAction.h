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
#pragma once // LeaveRequestAction.h
#include "livekit/signaling/LiveKitSignalingExport.h"
#include <string>

namespace LiveKitCpp
{

// indicates action clients should take on receiving this message
enum class LeaveRequestAction
{
    Disconnect = 0,  // should disconnect
    Resume = 1,      // should attempt a resume with `reconnect=1` in join URL
    Reconnect = 2,   // should attempt a reconnect, i. e. no `reconnect=1`
};

LIVEKIT_SIGNALING_API std::string toString(LeaveRequestAction action);

} // namespace LiveKitCpp
