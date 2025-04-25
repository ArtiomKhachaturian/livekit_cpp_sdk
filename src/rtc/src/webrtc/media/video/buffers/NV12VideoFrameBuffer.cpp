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
#include "NV12VideoFrameBuffer.h"
#include "LibyuvImport.h"
#include "VideoUtils.h"
#include <api/video/i420_buffer.h>

namespace LiveKitCpp 
{

NV12VideoFrameBuffer::NV12VideoFrameBuffer(VideoFrameBufferPool framesPool)
    : Base(std::move(framesPool))
{
}

int NV12VideoFrameBuffer::StrideY() const
{
    return width();
}

int NV12VideoFrameBuffer::StrideUV() const
{
    const auto w = width();
    return w + w % 2;
}

const uint8_t* NV12VideoFrameBuffer::nv12DataUV(const uint8_t* buffer, int width, int height)
{
    return buffer + (width * height);
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> NV12VideoFrameBuffer::
    CropAndScale(int offsetX, int offsetY, int cropWidth,
                 int cropHeight, int scaledWidth, int scaledHeight)
{
    if (scaledWidth > 0 && scaledHeight > 0) {
        const auto width = this->width(), height = this->height();
        if (width > 0 && height > 0) {
            auto const strideY = StrideY(), strideUV = StrideUV();
            if (strideY > 0 && strideUV > 0) {
                const auto dataY = DataY();
                const auto dataUV = DataUV();
                if (dataY && dataUV) {
                    offsetX = std::max(0, offsetX);
                    offsetY = std::max(0, offsetY);
                    if (width - offsetX > 0 && height - offsetY > 0) {
                        if (const auto scaled = createNV12(scaledWidth, scaledHeight)) {
                            if (!scaleNV12(dataY, strideY, dataUV, strideUV,
                                           cropWidth, cropHeight,
                                           scaled->MutableDataY(), scaled->StrideY(),
                                           scaled->MutableDataUV(), scaled->StrideUV(),
                                           scaled->width(), scaled->height(),
                                           contentHint())) {
                                scaled->CropAndScaleFrom(*this, offsetX, offsetY, cropWidth, cropHeight);
                            }
                            return scaled;
                        }
                    }
                }
            }
        }
    }
    return Base::CropAndScale(offsetX, offsetY, cropWidth, cropHeight, scaledWidth, scaledHeight);
}

rtc::scoped_refptr<webrtc::I420BufferInterface> NV12VideoFrameBuffer::convertToI420() const
{
    if (auto i420 = createI420(width(), height())) {
        static thread_local webrtc::NV12ToI420Scaler scaler;
        scaler.NV12ToI420Scale(DataY(),
                               StrideY(),
                               DataUV(),
                               StrideUV(),
                               width(), height(),
                               i420->MutableDataY(), i420->StrideY(),
                               i420->MutableDataU(), i420->StrideU(),
                               i420->MutableDataV(), i420->StrideV(),
                               i420->width(), i420->height());
        return i420;
    }
    return nullptr;
}

} // namespace LiveKitCpp
