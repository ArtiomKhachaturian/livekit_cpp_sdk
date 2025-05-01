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
#pragma once // LocalMediaTrack.h
#include "livekit/rtc/media/NetworkPriority.h"

namespace LiveKitCpp
{

template <class TBaseTrack>
class LocalMediaTrack : public TBaseTrack
{
public:
    // RTP
    // The relative DiffServ Code Point priority for this encoding, allowing
    // packets to be marked relatively higher or lower without affecting
    // bandwidth allocations. See https://w3c.github.io/webrtc-dscp-exp/ .
    virtual NetworkPriority networkPriority() const = 0;
    virtual void setNetworkPriority(NetworkPriority priority) = 0;
    // The relative bitrate priority of this encoding. Currently this is
    // implemented for the entire rtp sender by using the value of the first
    // encoding parameter.
    // See: https://w3c.github.io/webrtc-priority/#enumdef-rtcprioritytype
    // "very-low" = 0.5
    // "low" = 1.0
    // "medium" = 2.0
    // "high" = 4.0
    virtual double bitratePriority() const = 0;
    virtual void setBitratePriority(double priority) = 0;
};

} // namespace LiveKitCpp
