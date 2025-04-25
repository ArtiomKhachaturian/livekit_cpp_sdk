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
#pragma once // VideoFrame.h
#include "livekit/rtc/LiveKitRtcExport.h"
#include "livekit/rtc/media/VideoFrameType.h"
#include <memory>

namespace LiveKitCpp
{

class LIVEKIT_RTC_API VideoFrame
{
public:
    virtual ~VideoFrame() = default;
    VideoFrameType type() const noexcept { return _type; }
    // 0, 90, 180, 270 (clockwise rotation)
    int rotation() const noexcept { return _rotation; }
    // returns the current time in microseconds, system monotonic clock, 
    int64_t timestampUs() const noexcept { return _timestampUs; }
    virtual size_t planesCount() const;
    virtual int stride() const { return stride(0U); }
    virtual const std::byte* data() const { return data(0U); }
    virtual int dataSize() const { return dataSize(0U); }
    virtual std::shared_ptr<VideoFrame> convertToI420() const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual int stride(size_t planeIndex) const = 0;
    virtual const std::byte* data(size_t planeIndex) const = 0;
    virtual int dataSize(size_t planeIndex) const = 0;
protected:
    // pass zero timestampUs for automatic timestamp
    VideoFrame(VideoFrameType type, int rotation, int64_t timestampUs = 0LL);
private:
    const VideoFrameType _type;
    const int _rotation;
    const int64_t _timestampUs;
};

} // namespace LiveKitCpp
