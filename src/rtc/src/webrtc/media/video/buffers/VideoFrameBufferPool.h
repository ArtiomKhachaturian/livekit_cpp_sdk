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
#pragma once // VideoFrameBufferPool.h
#include "livekit/rtc/media/VideoContentHint.h"
#include <api/scoped_refptr.h>
#include <api/video/i010_buffer.h>
#include <api/video/i210_buffer.h>
#include <api/video/i410_buffer.h>
#include <api/video/i420_buffer.h>
#include <api/video/i422_buffer.h>
#include <api/video/i444_buffer.h>
#include <api/video/nv12_buffer.h>
#include <memory>

namespace LiveKitCpp
{

class VideoFrameBufferPoolSource;
class RgbVideoFrameBuffer;
enum class VideoFrameType;

class VideoFrameBufferPool
{
public:
    VideoFrameBufferPool(std::weak_ptr<VideoFrameBufferPoolSource> source = {});
    VideoFrameBufferPool(const VideoFrameBufferPool&) = default;
    VideoFrameBufferPool(VideoFrameBufferPool&&) = default;
    VideoFrameBufferPool& operator = (const VideoFrameBufferPool&) = default;
    VideoFrameBufferPool& operator = (VideoFrameBufferPool&&) = default;
    VideoContentHint contentHint() const;
    webrtc::scoped_refptr<webrtc::I420Buffer> createI420(int width, int height) const;
    webrtc::scoped_refptr<webrtc::I422Buffer> createI422(int width, int height) const;
    webrtc::scoped_refptr<webrtc::I444Buffer> createI444(int width, int height) const;
    webrtc::scoped_refptr<webrtc::I010Buffer> createI010(int width, int height) const;
    webrtc::scoped_refptr<webrtc::I210Buffer> createI210(int width, int height) const;
    webrtc::scoped_refptr<webrtc::I410Buffer> createI410(int width, int height) const;
    webrtc::scoped_refptr<webrtc::NV12Buffer> createNV12(int width, int height) const;
    webrtc::scoped_refptr<RgbVideoFrameBuffer> createRgb(int width, int height,
                                                         VideoFrameType rgbFormat,
                                                         int stride = 0) const;
private:
    template <class TBuffer, class TMethod, typename... Args>
    webrtc::scoped_refptr<TBuffer> create(TMethod method, int width,
                                          int height, Args&&... args) const;
private:
    std::weak_ptr<VideoFrameBufferPoolSource> _source;
};
	
} // namespace LiveKitCpp
