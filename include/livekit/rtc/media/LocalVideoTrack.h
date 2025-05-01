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
#pragma once // LocalVideoTrack.h
#include "livekit/rtc/media/LocalMediaTrack.h"
#include "livekit/rtc/media/VideoTrack.h"
#include "livekit/rtc/media/VideoOptions.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include "livekit/rtc/media/DegradationPreference.h"
#include <optional>

namespace LiveKitCpp
{

class LocalVideoFilterPin;

class LocalVideoTrack : public LocalMediaTrack<VideoTrack>
{
public:
    virtual void setDeviceInfo(MediaDeviceInfo info = {}) = 0;
    virtual MediaDeviceInfo deviceInfo() const = 0;
    virtual void setOptions(VideoOptions options = {}) = 0;
    virtual VideoOptions options() const = 0;
    virtual void setFilter(LocalVideoFilterPin* inputPin) = 0;
    // When bandwidth is constrained and the RtpSender needs to choose between
    // degrading resolution or degrading framerate, degradationPreference
    // indicates which is preferred. Only for video tracks.
    virtual DegradationPreference degradationPreference() const = 0;
    virtual void setDegradationPreference(DegradationPreference preference) = 0;
    // If set, this represents the Transport Independent Application Specific
    // maximum bandwidth defined in RFC3890. If unset, there is no maximum
    // bitrate. Currently this is implemented for the entire rtp sender by using
    // the value of the first encoding parameter.
    virtual std::optional<int> maxBitrateBps() const = 0;
    virtual void setMaxBitrateBps(const std::optional<int>& bps) = 0;
    // Specifies the minimum bitrate in bps for video.
    virtual std::optional<int> minBitrateBps() const = 0;
    virtual void setMinBitrateBps(const std::optional<int>& bps) = 0;
    // Specifies the maximum framerate in fps for video.
    virtual std::optional<int> maxFramerate() const = 0;
    virtual void setMaxFramerate(const std::optional<int>& fps) = 0;
};

} // namespace LiveKitCpp
