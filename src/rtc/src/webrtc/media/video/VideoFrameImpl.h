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
#include "VideoFrameBufferPool.h"
#include "livekit/rtc/media/VideoFrame.h"
#include <api/video/video_frame.h>
#include <optional>

namespace LiveKitCpp
{

class VideoFrameImpl : public VideoFrame
{
public:
    static std::shared_ptr<VideoFrame> create(const webrtc::VideoFrame& frame);
    static std::shared_ptr<VideoFrame> create(rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                                              webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                                              int64_t timestampUs = 0);
    static std::optional<webrtc::VideoFrame> create(const std::shared_ptr<VideoFrame>& frame,
                                                    VideoFrameBufferPool framesPool = {});
    // impl. of VideoFrame
    std::shared_ptr<VideoFrame> convertToI420() const final;
    int width() const final;
    int height() const final;
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    int dataSize(size_t planeIndex) const final;
private:
    VideoFrameImpl(VideoFrameType type, int rotation, int64_t timestampUs,
                   rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer);
    static std::optional<VideoFrameType> detectType(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer);
    static int mapFromRtc(webrtc::VideoRotation rotation);
    static webrtc::VideoRotation mapToRtc(int rotation);
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer> map(rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer);
    static int stride(size_t planeIndex, const webrtc::PlanarYuvBuffer* buffer);
    static int stride(size_t planeIndex, const webrtc::BiplanarYuvBuffer* buffer);
    static const std::byte* data(size_t planeIndex, const webrtc::PlanarYuv8Buffer* buffer);
    static const std::byte* data(size_t planeIndex, const webrtc::PlanarYuv16BBuffer* buffer);
    static const std::byte* data(size_t planeIndex, const webrtc::BiplanarYuv8Buffer* buffer);
    static int dataSizeI420(size_t planeIndex, int width, int height);
    static int dataSizeI422(size_t planeIndex, int width, int height);
    static int dataSizeI444(size_t planeIndex, int width, int height);
    static int dataSizeI010(size_t planeIndex, int width, int height);
    static int dataSizeI210(size_t planeIndex, int width, int height);
    static int dataSizeI410(size_t planeIndex, int width, int height);
    static int dataSizeNV12(size_t planeIndex, int width, int height);
private:
    const rtc::scoped_refptr<webrtc::VideoFrameBuffer> _buffer;
};

} // namespace LiveKitCpp
