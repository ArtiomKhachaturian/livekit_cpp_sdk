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
#pragma once // RgbVideoFrameBuffer.h
#include "VideoFrameBuffer.h"
#include "NativeVideoFrameBuffer.h"

namespace LiveKitCpp
{

class RgbVideoFrameBuffer : public VideoFrameBuffer<NativeVideoFrameBuffer>
{
    using Base = VideoFrameBuffer<NativeVideoFrameBuffer>;
public:
    static int bytesPerPixel(VideoFrameType rgbFormat);
    int bytesPerPixel() const { return bytesPerPixel(nativeType()); }
    // impl. of NativeVideoFrameBuffer
    VideoFrameType nativeType() const final { return _rgbFormat; }
protected:
    RgbVideoFrameBuffer(VideoFrameBufferPool framesPool, VideoFrameType rgbFormat);
    virtual rtc::scoped_refptr<RgbVideoFrameBuffer> create(int width, int height);
    // overrides of webrtc::VideoFrameBuffer
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> CropAndScale(int offsetX,
                                                              int offsetY,
                                                              int cropWidth,
                                                              int cropHeight,
                                                              int scaledWidth,
                                                              int scaledHeight) override;
    // impl. of VideoFrameBuffer<>
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
private:
    // common
    static bool scale(VideoFrameType type, const std::byte* srcRGB,
                      int srcStrideRGB, int srcWidth, int srcHeight,
                      std::byte* dstRGB, int dstStrideRGB,
                      int dstWidth, int dstHeight);
    // 24bpp
    static bool scale24bpp(const std::byte* srcRGB, int srcStrideRGB,
                           int srcWidth, int srcHeight,
                           std::byte* dstRGB, int dstStrideRGB,
                           int dstWidth, int dstHeight);
    // 32bpp
    static bool scale32bpp(const std::byte* srcARGB, int srcStrideARGB,
                           int srcWidth, int srcHeight,
                           std::byte* dstARGB, int dstStrideARGB,
                           int dstWidth, int dstHeight);
private:
    const VideoFrameType _rgbFormat;
};
	
} // namespace LiveKitCpp
