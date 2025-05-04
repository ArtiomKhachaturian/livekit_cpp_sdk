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
#pragma once // NV12VideoFrameBuffer.h
#include "VideoFrameBuffer.h"

namespace webrtc {
class NV12Buffer;
}

namespace LiveKitCpp 
{

class NV12VideoFrameBuffer : public VideoFrameBuffer<webrtc::NV12BufferInterface>
{
    using Base = VideoFrameBuffer<webrtc::NV12BufferInterface>;
public:
    static const uint8_t* nv12DataY(const uint8_t* buffer) { return buffer; }
    static const uint8_t* nv12DataUV(const uint8_t* buffer, int width, int height);
    static webrtc::scoped_refptr<webrtc::NV12BufferInterface>
        toNV12(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer,
               const VideoFrameBufferPool& pool = {});
    // overrides of webrtc::VideoFrameBuffer
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> CropAndScale(int offsetX,
                                                              int offsetY,
                                                              int cropWidth,
                                                              int cropHeight,
                                                              int scaledWidth,
                                                              int scaledHeight) override;
    // impl. of webrtc::NV12BufferInterface
    Type type() const final { return Type::kNV12; }
    int StrideY() const override;
    int StrideUV() const override;
protected:
    NV12VideoFrameBuffer(VideoFrameBufferPool framesPool);
private:
    // impl. of VideoFrameBuffer
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

} // namespace LiveKitCpp
