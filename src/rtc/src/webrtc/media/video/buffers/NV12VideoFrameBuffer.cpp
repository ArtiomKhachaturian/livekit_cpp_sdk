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
#include "RgbGenericVideoFrameBuffer.h"
#include "LibyuvImport.h"
#include "VideoUtils.h"
#include <api/video/i420_buffer.h>

namespace
{

using namespace LiveKitCpp;

webrtc::scoped_refptr<webrtc::NV12BufferInterface> i420ToNV12(const webrtc::I420BufferInterface* i420,
                                                              const VideoFrameBufferPool& pool);
webrtc::scoped_refptr<webrtc::NV12BufferInterface> i444ToNV12(const webrtc::I444BufferInterface* i444,
                                                              const VideoFrameBufferPool& pool);
webrtc::scoped_refptr<webrtc::NV12BufferInterface> rgbToNV12(RgbGenericVideoFrameBuffer* rgb,
                                                             const VideoFrameBufferPool& pool);

webrtc::VideoFrameBuffer::Type g_nv12Format[] = {webrtc::VideoFrameBuffer::Type::kNV12};

}

namespace LiveKitCpp 
{

NV12VideoFrameBuffer::NV12VideoFrameBuffer(VideoFrameBufferPool framesPool)
    : Base(std::move(framesPool))
{
}

bool NV12VideoFrameBuffer::consistent() const
{
    const auto w = width();
    return strideY(w) == StrideY() && strideUV(w) == StrideUV();
}

webrtc::scoped_refptr<webrtc::NV12BufferInterface> NV12VideoFrameBuffer::
    toNV12(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer,
           const VideoFrameBufferPool& pool)
{
    webrtc::scoped_refptr<webrtc::NV12BufferInterface> target;
    if (buffer) {
        switch (buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kNV12:
                target = const_cast<webrtc::NV12BufferInterface*>(buffer->GetNV12());
                break;
            case webrtc::VideoFrameBuffer::Type::kI420:
               target = i420ToNV12(buffer->GetI420(), pool);
               break;
           case webrtc::VideoFrameBuffer::Type::kI444:
               target = i444ToNV12(buffer->GetI444(), pool);
               break;
            case webrtc::VideoFrameBuffer::Type::kNative:
              if (const auto rgbBuffer = dynamic_cast<RgbGenericVideoFrameBuffer*>(buffer.get())) {
                  target = rgbToNV12(rgbBuffer, pool);
              }
              break;
          default:
              break;
        }
        if (!target) {
           if (webrtc::VideoFrameBuffer::Type::kNative == buffer->type()) {
               if (const auto mappedNV12 = buffer->GetMappedFrameBuffer(webrtc::MakeArrayView(&g_nv12Format[0], 1U))) {
                   target = dynamic_cast<webrtc::NV12BufferInterface*>(mappedNV12.get());
               }
           }
           if (!target) {
               if (const auto i420 = buffer->ToI420()) {
                   target = i420ToNV12(i420.get(), pool);
               }
           }
       }
    }
    return target;
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


namespace
{

webrtc::scoped_refptr<webrtc::NV12BufferInterface> i420ToNV12(const webrtc::I420BufferInterface* i420,
                                                              const VideoFrameBufferPool& pool)
{
    if (i420) {
        const auto nv12 = pool.createNV12(i420->width(), i420->height());
        if (nv12 && 0 == libyuv::I420ToNV12(i420->DataY(), i420->StrideY(),
                                            i420->DataU(), i420->StrideU(),
                                            i420->DataV(), i420->StrideV(),
                                            nv12->MutableDataY(), nv12->StrideY(),
                                            nv12->MutableDataUV(), nv12->StrideUV(),
                                            nv12->width(), nv12->height())) {
            return nv12;
        }
    }
    return {};
}

webrtc::scoped_refptr<webrtc::NV12BufferInterface> i444ToNV12(const webrtc::I444BufferInterface* i444,
                                                              const VideoFrameBufferPool& pool)
{
    if (i444) {
        const auto nv12 = pool.createNV12(i444->width(), i444->height());
        if (nv12 && 0 == libyuv::I444ToNV12(i444->DataY(), i444->StrideY(),
                                            i444->DataU(), i444->StrideU(),
                                            i444->DataV(), i444->StrideV(),
                                            nv12->MutableDataY(), nv12->StrideY(),
                                            nv12->MutableDataUV(), nv12->StrideUV(),
                                            nv12->width(), nv12->height())) {
            return nv12;
        }
    }
    return {};
}

webrtc::scoped_refptr<webrtc::NV12BufferInterface> rgbToNV12(RgbGenericVideoFrameBuffer* rgb,
                                                             const VideoFrameBufferPool& pool)
{
    if (rgb) {
        const auto format = rgb->nativeType();
        bool useI420Mapping = VideoFrameType::RGB24 == format || VideoFrameType::BGR24 == format;
        if (!useI420Mapping) {
            decltype(&libyuv::ARGBToNV12) func = nullptr;
            switch (format) {
                case VideoFrameType::BGRA32:
                    func = &libyuv::ARGBToNV12;
                    break;
                case VideoFrameType::RGBA32:
                    func = &libyuv::ABGRToNV12;
                    break;
                // no conversion routines for ARGB32 & ABGR32
                case VideoFrameType::ARGB32:
                case VideoFrameType::ABGR32:
                    break;
                default:
                    break;
            }
            if (func) {
                const auto nv12 = pool.createNV12(rgb->width(), rgb->height());
                if (nv12) {
                    useI420Mapping = 0 != func(reinterpret_cast<const uint8_t*>(rgb->data(0U)),
                                               rgb->stride(0U),
                                               nv12->MutableDataY(), nv12->StrideY(),
                                               nv12->MutableDataUV(), nv12->StrideUV(),
                                               nv12->width(), nv12->height());
                    if (!useI420Mapping) {
                        return nv12;
                    }
                }
            } else {
                useI420Mapping = true;
            }
        }
        if (useI420Mapping) {
            if (const auto i420 = rgb->ToI420()) {
                return i420ToNV12(i420.get(), pool);
            }
        }
    }
    return nullptr;
}

}
