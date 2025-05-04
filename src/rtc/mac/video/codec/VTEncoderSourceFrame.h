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
#pragma once // VTEncoderSourceFrame.h
#include "CVPixelBufferAutoRelease.h"
#include "VideoFrameBufferPool.h"
#include <api/rtc_error.h>
#include <api/video/video_frame.h>
#include <optional>
#include <memory>

namespace webrtc {
class VideoFrame;
class VideoFrameBuffer;
} // namespace webrtc

namespace LiveKitCpp
{

class VTEncoderSourceFrame
{
    using PixelBuffer = webrtc::RTCErrorOr<CVPixelBufferAutoRelease>;
public:
    VTEncoderSourceFrame() = default;
    ~VTEncoderSourceFrame();
    OSType pixelFormat() const;
    const auto& mappedBuffer() const noexcept { return _mappedBuffer; } // no retain
    bool valid() const { return _mappedBuffer.valid(); }
    explicit operator bool() const { return valid(); }
    webrtc::VideoRotation rotation() const { return _frame ? _frame->rotation() : webrtc::VideoRotation::kVideoRotation_0; }
    // Get frame width.
    int width() const { return _frame ? _frame->width() : 0; }
    // Get frame height.
    int height() const { return _frame ? _frame->height() : 0; }
    uint32_t timestampRtp() const { return _frame ? _frame->rtp_timestamp() : 0U; }
    int64_t timestampUs() const { return _frame ? _frame->timestamp_us() : 0LL; }
    int64_t renderTimeMs() const { return _frame ? _frame->render_time_ms() : 0LL; }
    int64_t startTimestampMs() const { return _startTimestampMs; }
    int64_t finishTimestampMs() const { return _finishTimestampMs; }
    void setStartTimestamp(int64_t timestampMs = currentTimestampMs());
    void setFinishTimestamp(int64_t timestampMs = currentTimestampMs());
    static int64_t currentTimestampMs();
    static webrtc::RTCErrorOr<VTEncoderSourceFrame> create(const webrtc::VideoFrame& frame,
                                                           const VideoFrameBufferPool& framesPool = {});
protected:
    VTEncoderSourceFrame(const webrtc::VideoFrame& frame, CVPixelBufferAutoRelease mappedBuffer);
private:
    static PixelBuffer convertToPixelBuffer(const webrtc::VideoFrame& frame,
                                            const VideoFrameBufferPool& framesPool = {});
private:
    // maximum number of planes supported by this implementation
    static inline const size_t _maxPlanes = 2U; // 1 - RGB32, 2 - NV12
    std::optional<webrtc::VideoFrame> _frame;
    CVPixelBufferAutoRelease _mappedBuffer;
    int64_t _startTimestampMs = 0LL;
    int64_t _finishTimestampMs = 0LL;
};

} // namespace LiveKitCpp
