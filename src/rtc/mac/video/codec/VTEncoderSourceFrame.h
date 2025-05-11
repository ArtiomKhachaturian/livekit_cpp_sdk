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
#ifdef USE_PLATFORM_ENCODERS
#include "CompletionStatusOr.h"
#include "CVPixelBufferAutoRelease.h"
#include "VideoFrameBufferPool.h"
#include "VideoFrameInfo.h"
#include <api/video/video_frame.h>
#include <optional>
#include <memory>

namespace webrtc {
class VideoFrame;
class VideoFrameBuffer;
} // namespace webrtc

namespace LiveKitCpp
{

class VTEncoderSourceFrame : public VideoFrameInfo
{
    using PixelBuffer = CompletionStatusOr<CVPixelBufferAutoRelease>;
    class Planes;
public:
    VTEncoderSourceFrame() = default;
    ~VTEncoderSourceFrame();
    OSType pixelFormat() const;
    const auto& mappedBuffer() const noexcept { return _mappedBuffer; } // no retain
    bool valid() const { return _mappedBuffer.valid(); }
    explicit operator bool() const { return valid(); }
    static CompletionStatusOr<VTEncoderSourceFrame> create(const webrtc::VideoFrame& frame,
                                                           const VideoFrameBufferPool& framesPool = {});
protected:
    VTEncoderSourceFrame(const webrtc::VideoFrame& frame, CVPixelBufferAutoRelease mappedBuffer);
private:
    static PixelBuffer convertToPixelBuffer(const webrtc::VideoFrame& frame,
                                            const VideoFrameBufferPool& framesPool = {});
private:
    // maximum number of planes supported by this implementation
    static inline const size_t _maxPlanes = 2U; // 1 - RGB32, 2 - NV12
    CVPixelBufferAutoRelease _mappedBuffer;
};

} // namespace LiveKitCpp
#endif
