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
#include "CoreVideoPixelBuffer.h"
#include "CVPixelBufferAutoRelease.h"
#include "NV12VideoFrameBuffer.h"
#include "RgbVideoFrameBuffer.h"
#include "VideoUtils.h"
#include <api/make_ref_counted.h>
#include <cassert>

namespace
{

using namespace LiveKitCpp;

class CVBufferAccessor
{
public:
    virtual CVPixelBufferRef buffer(bool retain) const = 0;
protected:
    virtual ~CVBufferAccessor() = default;
};

template <class TBaseVideoBuffer>
class CVBuffer : public TBaseVideoBuffer, public CVBufferAccessor
{
public:
    ~CVBuffer();
    size_t cvDataSize() const { return _lockedBuffer.dataSize(); }
    size_t cvWidth() const { return _lockedBuffer.width(); }
    size_t cvWidth(size_t planeIndex) const { return _lockedBuffer.width(planeIndex); }
    size_t cvHeight() const { return _lockedBuffer.height(); }
    size_t cvHeight(size_t planeIndex) const { return _lockedBuffer.height(planeIndex); }
    size_t cvStride(size_t planeIndex) const { return _lockedBuffer.stride(planeIndex); }
    size_t cvStride() const { return _lockedBuffer.stride(); }
    uint8_t* cvData(size_t planeIndex) const { return _lockedBuffer.planeAddress(planeIndex); }
    uint8_t* cvData() const { return _lockedBuffer.baseAddress(); }
    // impl. of CVBufferAccessor
    CVPixelBufferRef buffer(bool retain) const final { return _lockedBuffer.ref(retain); }
protected:
    template <class... Args>
    CVBuffer(CVPixelBufferAutoRelease lockedBuffer, Args&&... args);
private:
    CVPixelBufferAutoRelease _lockedBuffer;
};

class NV12Buffer : public CVBuffer<NV12VideoFrameBuffer>
{
    using BaseClass = CVBuffer<NV12VideoFrameBuffer>;
public:
    NV12Buffer(CVPixelBufferAutoRelease lockedBuffer);
    // impl. of webrtc::NV12BufferInterface
    const uint8_t* DataY() const final { return cvData(0U); }
    const uint8_t* DataUV() const final { return cvData(1U); }
    int StrideY() const final { return static_cast<int>(cvStride(0U)); }
    int StrideUV() const final { return static_cast<int>(cvStride(1U)); }
    int width() const final { return static_cast<int>(cvWidth()); }
    int height() const final { return static_cast<int>(cvHeight()); }
};

class RGBBuffer : public CVBuffer<RgbVideoFrameBuffer>
{
    using BaseClass = CVBuffer<RgbVideoFrameBuffer>;
public:
    RGBBuffer(CVPixelBufferAutoRelease lockedBuffer, VideoFrameType rgbFormat);
    // impl. of NativeVideoFrameBuffer
    int width() const final { return static_cast<int>(cvWidth()); }
    int height() const final { return static_cast<int>(cvHeight()); }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
};

}

namespace LiveKitCpp
{

bool CoreVideoPixelBuffer::supported(CVPixelBufferRef buffer)
{
    return buffer && isSupportedFormat(CVPixelBufferAutoRelease::pixelFormat(buffer));
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> CoreVideoPixelBuffer::
    create(CVPixelBufferRef buffer, bool retain)
{
    if (buffer) {
        const auto format = CVPixelBufferAutoRelease::pixelFormat(buffer);
        if (isSupportedFormat(format)) {
            CVPixelBufferAutoRelease lockedBuffer(buffer, retain);
            if (lockedBuffer.lock()) {
                if (isNV12Format(format)) {
                    return rtc::make_ref_counted<NV12Buffer>(std::move(lockedBuffer));
                }
                if (isRGBFormat(format)) {
                    std::optional<VideoFrameType> rgbFormat;
                    switch (format) {
                        case formatRGB24(): // 24bpp
                            rgbFormat = VideoFrameType::RGB24;
                            break;
                        case formatBGR24(): // 24bpp
                            rgbFormat = VideoFrameType::BGR24;
                            break;
                        case formatBGRA32(): // 32bpp
                            rgbFormat = VideoFrameType::BGRA32;
                            break;
                        case formatARGB32(): // 32bpp
                            rgbFormat = VideoFrameType::ARGB32;
                            break;
                        case formatRGBA32(): // 32bpp
                            rgbFormat = VideoFrameType::RGBA32;
                            break;
                        default:
                            break;
                    }
                    if (rgbFormat.has_value()) {
                        return rtc::make_ref_counted<RGBBuffer>(std::move(lockedBuffer), rgbFormat.value());
                    }
                }
                lockedBuffer.unlock();
            }
        }
    }
    return nullptr;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> CoreVideoPixelBuffer::
    createFromSampleBuffer(CMSampleBufferRef buffer)
{
    return create(CVPixelBufferAutoRelease::imageBuffer(buffer), true);
}

CVPixelBufferRef CoreVideoPixelBuffer::pixelBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& videoPixelBuffer,
                                                   bool retain)
{
    if (const auto accessor = dynamic_cast<const CVBufferAccessor*>(videoPixelBuffer.get())) {
        return accessor->buffer(retain);
    }
    return nullptr;
}

} // namespace LiveKitCpp

namespace
{

template <class TBaseVideoBuffer>
template <class... Args>
CVBuffer<TBaseVideoBuffer>::CVBuffer(CVPixelBufferAutoRelease lockedBuffer, Args&&... args)
    : TBaseVideoBuffer(std::forward<Args>(args)...)
    , _lockedBuffer(std::move(lockedBuffer))
{
}

template <class TBaseVideoBuffer>
CVBuffer<TBaseVideoBuffer>::~CVBuffer()
{
    _lockedBuffer.unlock();
}

NV12Buffer::NV12Buffer(CVPixelBufferAutoRelease lockedBuffer)
    : BaseClass(std::move(lockedBuffer))
{
}

RGBBuffer::RGBBuffer(CVPixelBufferAutoRelease lockedBuffer, VideoFrameType rgbFormat)
    : BaseClass(std::move(lockedBuffer), rgbFormat)
{
}

int RGBBuffer::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return static_cast<int>(cvStride(0U));
    }
    return 0;
}

const std::byte* RGBBuffer::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(cvData(0U));
    }
    return nullptr;
}

}
