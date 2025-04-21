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
#include "VideoFrameBuffer.h"
#include "VideoUtils.h"
#include "NativeVideoFrameBuffer.h"
#include <api/video/nv12_buffer.h>
#include <api/video/i420_buffer.h>
#include <api/make_ref_counted.h>
#include <third_party/libyuv/include/libyuv/convert.h>
#include <third_party/libyuv/include/libyuv/scale.h>
#include <cassert>

namespace
{

using namespace LiveKitCpp;

class CoreVideoPixelBufferAccessor
{
public:
    virtual CVPixelBufferRef buffer(bool retain) const = 0;
protected:
    virtual ~CoreVideoPixelBufferAccessor() = default;
};

template <class TBaseVideoBuffer>
class CoreVideoPixelBufferHolder : public VideoFrameBuffer<TBaseVideoBuffer>,
                                   public CoreVideoPixelBufferAccessor
{
public:
    ~CoreVideoPixelBufferHolder();
    size_t cvDataSize() const { return _lockedBuffer.dataSize(); }
    size_t cvWidth() const { return _lockedBuffer.width(); }
    size_t cvWidth(size_t planeIndex) const { return _lockedBuffer.width(planeIndex); }
    size_t cvHeight() const { return _lockedBuffer.height(); }
    size_t cvHeight(size_t planeIndex) const { return _lockedBuffer.height(planeIndex); }
    size_t cvStride(size_t planeIndex) const { return _lockedBuffer.stride(planeIndex); }
    size_t cvStride() const { return _lockedBuffer.stride(); }
    uint8_t* cvData(size_t planeIndex) const { return _lockedBuffer.planeAddress(planeIndex); }
    uint8_t* cvData() const { return _lockedBuffer.baseAddress(); }
    // impl. of CoreVideoPixelBufferAccessor
    CVPixelBufferRef buffer(bool retain) const final { return _lockedBuffer.ref(retain); }
protected:
    template <class... AdditionalArgs>
    CoreVideoPixelBufferHolder(CVPixelBufferAutoRelease lockedBuffer, AdditionalArgs&&... additionalArgs);
    virtual rtc::scoped_refptr<webrtc::VideoFrameBuffer>
        allocateBuffer(int /*width*/, int /*height*/) const { return nullptr; }
private:
    CVPixelBufferAutoRelease _lockedBuffer;
};

class NV12PixelBuffer : public CoreVideoPixelBufferHolder<webrtc::NV12BufferInterface>
{
    using BaseClass = CoreVideoPixelBufferHolder<webrtc::NV12BufferInterface>;
public:
    NV12PixelBuffer(CVPixelBufferAutoRelease lockedBuffer);
    // impl. of webrtc::NV12BufferInterface
    const uint8_t* DataY() const final { return cvData(0U); }
    const uint8_t* DataUV() const final { return cvData(1U); }
    int StrideY() const final { return static_cast<int>(cvStride(0U)); }
    int StrideUV() const final { return static_cast<int>(cvStride(1U)); }
    int width() const final { return static_cast<int>(cvWidth()); }
    int height() const final { return static_cast<int>(cvHeight()); }
protected:
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

class RGBPixelBuffer : public CoreVideoPixelBufferHolder<NativeVideoFrameBuffer>
{
    using BaseClass = CoreVideoPixelBufferHolder<NativeVideoFrameBuffer>;
public:
    RGBPixelBuffer(CVPixelBufferAutoRelease lockedBuffer, VideoFrameType rgbFormat);
    // impl. of NativeVideoFrameBuffer
    VideoFrameType nativeType() const final { return _rgbFormat; }
    int width() const final { return static_cast<int>(cvWidth()); }
    int height() const final { return static_cast<int>(cvHeight()); }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    int dataSize(size_t planeIndex) const final;
protected:
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
private:
    const VideoFrameType _rgbFormat;
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
                    return rtc::make_ref_counted<NV12PixelBuffer>(std::move(lockedBuffer));
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
                        return rtc::make_ref_counted<RGBPixelBuffer>(std::move(lockedBuffer), rgbFormat.value());
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
    if (const auto accessor = dynamic_cast<const CoreVideoPixelBufferAccessor*>(videoPixelBuffer.get())) {
        return accessor->buffer(retain);
    }
    return nullptr;
}

} // namespace LiveKitCpp

namespace
{

template <class TBaseVideoBuffer> template <class... AdditionalArgs>
CoreVideoPixelBufferHolder<TBaseVideoBuffer>::CoreVideoPixelBufferHolder(CVPixelBufferAutoRelease lockedBuffer,
                                                                         AdditionalArgs&&... additionalArgs)
    : VideoFrameBuffer<TBaseVideoBuffer>(std::forward<AdditionalArgs>(additionalArgs)...)
    , _lockedBuffer(std::move(lockedBuffer))
{
}

template <class TBaseVideoBuffer>
CoreVideoPixelBufferHolder<TBaseVideoBuffer>::~CoreVideoPixelBufferHolder()
{
    _lockedBuffer.unlock();
}

NV12PixelBuffer::NV12PixelBuffer(CVPixelBufferAutoRelease lockedBuffer)
    : BaseClass(std::move(lockedBuffer))
{
}

rtc::scoped_refptr<webrtc::I420BufferInterface> NV12PixelBuffer::convertToI420() const
{
    if (auto i420 = createI420(width(), height())) {
        if (0 == libyuv::NV12ToI420(DataY(), StrideY(), DataUV(), StrideUV(),
                                    i420->MutableDataY(), i420->StrideY(),
                                    i420->MutableDataU(), i420->StrideU(),
                                    i420->MutableDataV(), i420->StrideV(),
                                    width(), height())) {
            return i420;
        }
    }
    return {};
}

RGBPixelBuffer::RGBPixelBuffer(CVPixelBufferAutoRelease lockedBuffer, VideoFrameType rgbFormat)
    : BaseClass(std::move(lockedBuffer))
    , _rgbFormat(rgbFormat)
{
}

int RGBPixelBuffer::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return static_cast<int>(cvStride(0U));
    }
    return 0;
}

const std::byte* RGBPixelBuffer::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(cvData(0U));
    }
    return nullptr;
}

int RGBPixelBuffer::dataSize(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return static_cast<int>(cvStride(0U) * cvHeight());;
    }
    return 0;
}

rtc::scoped_refptr<webrtc::I420BufferInterface> RGBPixelBuffer::convertToI420() const
{
    const auto w = width(), h = height();
    const auto stride = cvStride(0U);
    const auto rgb = cvData(0U);
    if (w > 0 && h > 0 && stride && rgb) {
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
            if (i420 && 0 == func(rgb, static_cast<int>(stride),
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

}
