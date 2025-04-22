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
#include <api/make_ref_counted.h>

namespace LiveKitCpp
{

RgbVideoFrameBuffer::RgbVideoFrameBuffer(VideoFrameType rgbFormat,
                                         int width, int height, int stride,
                                         VideoFrameBufferPool framesPool)
    : RgbGenericVideoFrameBuffer(rgbFormat, std::move(framesPool))
    , _width(width)
    , _height(height)
    , _stride(stride > 0 ? stride : bytesPerPixel(rgbFormat) * _width)
    , _data(allocate(_stride, _height))
{
}

int RgbVideoFrameBuffer::stride(size_t planeIndex) const
{
    return 0U == planeIndex ? _stride : 0;
}

const std::byte* RgbVideoFrameBuffer::data(size_t planeIndex) const
{
    return 0U == planeIndex ? _data.get() : nullptr;
}

rtc::scoped_refptr<RgbVideoFrameBuffer> RgbVideoFrameBuffer::Create(int width,
                                                                    int height,
                                                                    VideoFrameType rgbFormat,
                                                                    int stride,
                                                                    VideoFrameBufferPool framesPool)
{
    if (width > 0 && height > 0 && isRGB(rgbFormat)) {
        return webrtc::make_ref_counted<RgbVideoFrameBuffer>(rgbFormat, width, height,
                                                             stride, std::move(framesPool));
    }
    return {};
}

std::unique_ptr<std::byte[]> RgbVideoFrameBuffer::allocate(int stride, int height)
{
    std::unique_ptr<std::byte[]> data;
    if (stride > 0 && height > 0) {
        data.reset(new std::byte[stride * height]);
    }
    return data;
}

} // namespace LiveKitCpp
