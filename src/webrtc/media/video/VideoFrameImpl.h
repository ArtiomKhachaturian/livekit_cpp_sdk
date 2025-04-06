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
#pragma once // VideoFrameImpl.h
#include "media/VideoFrame.h"
#include <api/video/video_frame.h>

namespace LiveKitCpp
{

class VideoFrameImpl : public VideoFrame
{
public:
    static std::shared_ptr<VideoFrame> create(const webrtc::VideoFrame& frame);
    // impl. of VideoFrame
    std::shared_ptr<VideoFrame> convertToI420() const final;
    int width() const final;
    int height() const final;
    int stride(size_t planeIndex) const final;
    const void* data(size_t planeIndex) const final;
private:
    VideoFrameImpl(int rotation, rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer);
    static VideoFrameType detectType(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer);
    static int detectRotation(const webrtc::VideoFrame& frame);
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer> map(rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer);
    static int stride(size_t planeIndex, const webrtc::PlanarYuvBuffer* buffer);
    static int stride(size_t planeIndex, const webrtc::BiplanarYuvBuffer* buffer);
    static const void* data(size_t planeIndex, const webrtc::PlanarYuv8Buffer* buffer);
    static const void* data(size_t planeIndex, const webrtc::PlanarYuv16BBuffer* buffer);
    static const void* data(size_t planeIndex, const webrtc::BiplanarYuv8Buffer* buffer);
private:
    const rtc::scoped_refptr<webrtc::VideoFrameBuffer> _buffer;
};

} // namespace LiveKitCpp
