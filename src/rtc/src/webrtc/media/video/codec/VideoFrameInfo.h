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
#pragma once // VideoFrameInfo.h
#include <api/video/video_frame.h>

namespace LiveKitCpp
{

class VideoFrameInfo
{
public:
    VideoFrameInfo() = default;
    VideoFrameInfo(int64_t renderTimeMs, int64_t timestampUs,
                   uint32_t timestampRtpMs, int width, int height,
                   webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                   int64_t startTimestampMs = currentTimestampMs());
    VideoFrameInfo(const webrtc::VideoFrame& frame, int64_t startTimestampMs = currentTimestampMs());
    virtual ~VideoFrameInfo() = default;
    bool valid() const { return timestampRtpMs() > 0U && width() > 0 && height() > 0; }
    int64_t renderTimeMs() const { return _renderTimeMs; }
    int64_t timestampUs() const { return _timestampUs; }
    uint32_t timestampRtpMs() const { return _timestampRtpMs; }
    int width() const { return _width; }
    int height() const { return _height; }
    webrtc::VideoRotation rotation() const { return _rotation; }
    int64_t startTimestampMs() const { return _startTimestampMs; }
    int64_t finishTimestampMs() const { return _finishTimestampMs; }
    void setStartTimestamp(int64_t timestampMs) { _startTimestampMs = timestampMs; }
    void setFinishTimestamp(int64_t timestampMs) { _finishTimestampMs = timestampMs; }
    void markStartTimestamp() { setStartTimestamp(currentTimestampMs()); }
    void markFinishTimestamp() { setFinishTimestamp(currentTimestampMs()); }
    static int64_t currentTimestampMs();
private:
    int64_t _renderTimeMs = 0ULL;
    int64_t _timestampUs = 0LL;
    uint32_t _timestampRtpMs = 0U; // ms
    int _width = 0;
    int _height = 0;
    webrtc::VideoRotation _rotation = webrtc::VideoRotation::kVideoRotation_0;
    int64_t _startTimestampMs = 0LL;
    int64_t _finishTimestampMs = 0LL;
};

} // namespace LiveKitCpp
