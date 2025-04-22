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
#include "RgbVideoFrameBuffer.h"
#include "DesktopFrameVideoBuffer.h"
#include <api/make_ref_counted.h>
#include <third_party/libyuv/include/libyuv/convert.h>
#include <third_party/libyuv/include/libyuv/scale_argb.h>
#include <third_party/libyuv/include/libyuv/scale_rgb.h>
#include <cassert>

namespace LiveKitCpp
{

RgbVideoFrameBuffer::RgbVideoFrameBuffer(VideoFrameBufferPool framesPool,
                                         VideoFrameType rgbFormat)
    : Base(std::move(framesPool))
    , _rgbFormat(rgbFormat)
{
    assert(bytesPerPixel() > 0);
}

int RgbVideoFrameBuffer::bytesPerPixel(VideoFrameType rgbFormat)
{
    switch (rgbFormat) {
        case VideoFrameType::RGB24:
        case VideoFrameType::BGR24:
            return 3;
        case VideoFrameType::BGRA32:
        case VideoFrameType::ARGB32:
        case VideoFrameType::RGBA32:
        case VideoFrameType::ABGR32:
            return 4;
        default:
            break;
    }
    return 0;
}

rtc::scoped_refptr<RgbVideoFrameBuffer> RgbVideoFrameBuffer::create(int width, int height)
{
    if (width > 0 && height > 0 && nativeType() == DesktopFrameVideoBuffer::rgbType()) {
        return webrtc::make_ref_counted<DesktopFrameVideoBuffer>(width, height, framesPool());
    }
    return {};
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> RgbVideoFrameBuffer::CropAndScale(int offsetX,
                                                                               int offsetY,
                                                                               int cropWidth,
                                                                               int cropHeight,
                                                                               int scaledWidth,
                                                                               int scaledHeight)
{
    if (scaledWidth > 0 && scaledHeight > 0) {
        const auto width = this->width(), height = this->height();
        if (width > 0 && height > 0) {
            const auto stride = this->stride(0U);
            if (stride > 0) {
                if (const auto data = this->data(0U)) {
                    offsetX = std::max(0, offsetX);
                    offsetY = std::max(0, offsetY);
                    if (width - offsetX > 0 && height - offsetY > 0) {
                        if (const auto scaled = create(scaledWidth, scaledHeight)) {
                            const auto source = data + (offsetX * bytesPerPixel()) + (offsetY * stride);
                            if (scale(nativeType(), source, stride,
                                      cropWidth, cropHeight,
                                      const_cast<std::byte*>(scaled->data(0U)),
                                      scaled->stride(0U), scaled->width(), scaled->height())) {
                                return scaled;
                            }
                        }
                    }
                }
            }
        }
    }
    return Base::CropAndScale(offsetX, offsetY, cropWidth, cropHeight, scaledWidth, scaledHeight);
}

rtc::scoped_refptr<webrtc::I420BufferInterface> RgbVideoFrameBuffer::convertToI420() const
{
    const auto w = width(), h = height();
    const auto stride = this->stride(0U);
    const auto rgb = data(0U);
    if (w > 0 && h > 0 && stride > 0 && rgb) {
        decltype(&libyuv::ARGBToI420) func = nullptr;
        switch (_rgbFormat) {
            case VideoFrameType::RGB24:
                func = &libyuv::RAWToI420;
                break;
            case VideoFrameType::BGR24:
                func = &libyuv::RGB24ToI420;
                break;
            case VideoFrameType::BGRA32:
                func = &libyuv::ARGBToI420;
                break;
            case VideoFrameType::ARGB32:
                func = &libyuv::BGRAToI420;
                break;
            case VideoFrameType::RGBA32:
                func = &libyuv::ABGRToI420;
                break;
            case VideoFrameType::ABGR32:
                func = &libyuv::RGBAToI420;
                break;
            default:
                assert(false);
                break;
        }
        if (func) {
            auto i420 = createI420(w, h);
            if (i420 && 0 == func(reinterpret_cast<const uint8_t*>(rgb),
                                  stride,
                                  i420->MutableDataY(),
                                  i420->StrideY(),
                                  i420->MutableDataU(),
                                  i420->StrideU(),
                                  i420->MutableDataV(),
                                  i420->StrideV(), w, h)) {
                return i420;
            }
        }
    }
    return {};
}

bool RgbVideoFrameBuffer::scale(VideoFrameType type, const std::byte* srcRGB,
                                int srcStrideRGB, int srcWidth, int srcHeight,
                                std::byte* dstRGB, int dstStrideRGB,
                                int dstWidth, int dstHeight)
{
    switch (type) {
        case VideoFrameType::RGB24:
        case VideoFrameType::BGR24:
            return scale24bpp(srcRGB, srcStrideRGB, srcWidth, srcHeight,
                              dstRGB, dstStrideRGB, dstWidth, dstHeight);
        case VideoFrameType::BGRA32:
        case VideoFrameType::ARGB32:
        case VideoFrameType::RGBA32:
        case VideoFrameType::ABGR32:
            return scale32bpp(srcRGB, srcStrideRGB, srcWidth, srcHeight,
                              dstRGB, dstStrideRGB, dstWidth, dstHeight);
        default:
            assert(false);
            break;
    }
    return false;
}

bool RgbVideoFrameBuffer::scale24bpp(const std::byte* srcRGB, int srcStrideRGB,
                                     int srcWidth, int srcHeight,
                                     std::byte* dstRGB, int dstStrideRGB,
                                     int dstWidth, int dstHeight)
{
    return 0 == libyuv::RGBScale(reinterpret_cast<const uint8_t*>(srcRGB),
                                 srcStrideRGB, srcWidth, srcHeight,
                                 reinterpret_cast<uint8_t*>(dstRGB),
                                 dstStrideRGB, dstWidth, dstHeight,
                                 libyuv::FilterMode::kFilterNone);
}

bool RgbVideoFrameBuffer::scale32bpp(const std::byte* srcARGB, int srcStrideARGB,
                                     int srcWidth, int srcHeight,
                                     std::byte* dstARGB, int dstStrideARGB,
                                     int dstWidth, int dstHeight)
{
    return 0 == libyuv::ARGBScale(reinterpret_cast<const uint8_t*>(srcARGB),
                                  srcStrideARGB, srcWidth, srcHeight,
                                  reinterpret_cast<uint8_t*>(dstARGB),
                                  dstStrideARGB, dstWidth, dstHeight,
                                  libyuv::FilterMode::kFilterNone);
}

} // namespace LiveKitCpp
