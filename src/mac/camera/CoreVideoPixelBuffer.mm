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
#ifdef WEBRTC_MAC
#include "CoreVideoPixelBuffer.h"
#include "CVPixelBufferAutoRelease.h"
#include "VTSupportedPixelFormats.h"
#include "VideoFrameBuffer.h"
#include <api/video/nv12_buffer.h>
#include <api/video/i420_buffer.h>
#include <api/make_ref_counted.h>
#include <third_party/libyuv/include/libyuv/convert.h>
#include <third_party/libyuv/include/libyuv/scale.h>

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
    const uint8_t* DataY() const final { return cvData(0UL); }
    const uint8_t* DataUV() const final { return cvData(1UL); }
    int StrideY() const final { return cvStride(0UL); }
    int StrideUV() const final { return cvStride(1UL); }
    int width() const final { return cvWidth(); }
    int height() const final { return cvHeight(); }
protected:
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
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

bool isNV12Format(OSType format)
{
    return format == pixelFormatNV12Full() || format == pixelFormatNV12Video();
}

bool isRGB24Format(OSType format)
{
    return format == pixelFormatRGB24() || format == pixelFormatBGR24();
}

bool isRGB32Format(OSType format)
{
    return format == pixelFormatBGRA32() || format == pixelFormatARGB32() || format == pixelFormatRGBA32();
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
    auto i420 = createI420(width(), height());
    if (i420) {
        libyuv::NV12ToI420(DataY(), StrideY(), DataUV(), StrideUV(),
                           i420->MutableDataY(), i420->StrideY(),
                           i420->MutableDataU(), i420->StrideU(),
                           i420->MutableDataV(), i420->StrideV(),
                           width(), height());
    }
    return i420;
}

}
#endif
