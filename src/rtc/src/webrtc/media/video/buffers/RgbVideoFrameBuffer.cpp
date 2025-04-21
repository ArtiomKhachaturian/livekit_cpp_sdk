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
#include <third_party/libyuv/include/libyuv/convert.h>

namespace LiveKitCpp
{

RgbVideoFrameBuffer::RgbVideoFrameBuffer(VideoFrameBufferPool framesPool,
                                         VideoFrameType rgbFormat)
    : VideoFrameBuffer<NativeVideoFrameBuffer>(std::move(framesPool))
    , _rgbFormat(rgbFormat)
{
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

} // namespace LiveKitCpp
