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
#include "VideoFrameInfo.h"
#include <rtc_base/time_utils.h>

namespace LiveKitCpp
{

VideoFrameInfo::VideoFrameInfo(int64_t renderTimeMs, int64_t timestampUs,
                               uint32_t timestampRtpMs, int64_t ntpTimeMs,
                               int width, int height,
                               webrtc::VideoRotation rotation,
                               int64_t startTimestampMs)
    : _renderTimeMs(renderTimeMs)
    , _timestampUs(timestampUs)
    , _timestampRtpMs(timestampRtpMs)
    , _ntpTimeMs(ntpTimeMs)
    , _width(width)
    , _height(height)
    , _rotation(rotation)
    , _startTimestampMs(startTimestampMs)
{
}

VideoFrameInfo::VideoFrameInfo(const webrtc::VideoFrame& frame, int64_t startTimestampMs)
    : VideoFrameInfo(frame.render_time_ms(), frame.timestamp_us(),
                     frame.rtp_timestamp(), frame.ntp_time_ms(), frame.width(),
                     frame.height(), frame.rotation(), startTimestampMs)
{
}

int64_t VideoFrameInfo::currentTimestampMs()
{
    return webrtc::TimeMillis();
}

} // namespace LiveKitCpp
