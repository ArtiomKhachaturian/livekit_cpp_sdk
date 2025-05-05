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
#include "IOSurfaceBuffer.h"
#include "CFAutoRelease.h"
#include "RgbGenericVideoFrameBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "VideoUtils.h"
#include <api/make_ref_counted.h>

namespace {

using namespace LiveKitCpp;

class IOSBufferAccessor
{
public:
    virtual IOSurfaceRef buffer(bool retain) const = 0;
protected:
    virtual ~IOSBufferAccessor() = default;
};

using IOSurfaceAutoRelease = CFAutoRelease<IOSurfaceRef>;

template<class TBaseVideoBuffer>
class IOSBuffer : public TBaseVideoBuffer, public IOSBufferAccessor
{
public:
    ~IOSBuffer();
    size_t surfaceWidth() const { return IOSurfaceGetWidth(_buffer.ref()); }
    size_t surfaceHeight() const { return IOSurfaceGetHeight(_buffer.ref()); }
    size_t surfaceStride() const { return IOSurfaceGetBytesPerRow(_buffer.ref()); }
    size_t surfaceStride(size_t planeIndex) const;
    const uint8_t* surfaceData() const;
    const uint8_t* surfaceData(size_t planeIndex) const;
    // impl. of IOSurfaceBufferAccessor
    IOSurfaceRef buffer(bool retain) const final { return _buffer.ref(retain); }
    // override of VideoFrameBuffer<>
    VideoContentHint contentHint() const final;
protected:
    template <class... Args>
    IOSBuffer(IOSurfaceAutoRelease buffer,
              std::optional<VideoContentHint> contentHint,
              Args&&... args);
private:
    const IOSurfaceAutoRelease _buffer;
    const std::optional<VideoContentHint> _contentHint;
};

class NV12Buffer : public IOSBuffer<NV12VideoFrameBuffer>
{
    using BaseClass = IOSBuffer<NV12VideoFrameBuffer>;
public:
    NV12Buffer(IOSurfaceAutoRelease buffer,
               VideoFrameBufferPool framesPool = {},
               std::optional<VideoContentHint> contentHint = {});
    // impl. of webrtc::NV12BufferInterface
    const uint8_t* DataY() const final { return surfaceData(0U); }
    const uint8_t* DataUV() const final { return surfaceData(1U); }
    int StrideY() const final { return static_cast<int>(surfaceStride(0U)); }
    int StrideUV() const final { return static_cast<int>(surfaceStride(1U)); }
    int width() const final { return static_cast<int>(surfaceWidth()); }
    int height() const final { return static_cast<int>(surfaceHeight()); }
    std::string storage_representation() const final;
};

class RGBBuffer : public IOSBuffer<RgbGenericVideoFrameBuffer>
{
    using BaseClass = IOSBuffer<RgbGenericVideoFrameBuffer>;
public:
    RGBBuffer(IOSurfaceAutoRelease buffer,
              VideoFrameType rgbFormat,
              VideoFrameBufferPool framesPool = {},
              std::optional<VideoContentHint> contentHint = {});
    // impl. of RgbVideoFrameBuffer
    int width() const final { return static_cast<int>(surfaceWidth()); }
    int height() const final { return static_cast<int>(surfaceHeight()); }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    std::string storage_representation() const final;
};

}

namespace LiveKitCpp
{

bool IOSurfaceBuffer::supported(IOSurfaceRef buffer)
{
    return buffer && isSupportedFormat(IOSurfaceGetPixelFormat(buffer));
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> IOSurfaceBuffer::
    create(IOSurfaceRef buffer, VideoFrameBufferPool framesPool,
           std::optional<VideoContentHint> contentHint,
           bool retain)
{
    if (buffer) {
        const auto format = IOSurfaceGetPixelFormat(buffer);
        if (isSupportedFormat(format)) {
            if (kIOReturnSuccess == IOSurfaceLock(buffer, kIOSurfaceLockReadOnly, nullptr)) {
                if (isNV12Format(format)) {
                    auto nv12 = rtc::make_ref_counted<NV12Buffer>(IOSurfaceAutoRelease(buffer, retain),
                                                                  std::move(framesPool),
                                                                  std::move(contentHint));
                     if (!nv12->consistent()) {
                         return nv12->ToI420();
                     }
                     return nv12;
                }
                if (isRGB24Format(format)) {
                    std::optional<VideoFrameType> rgbFormat;
                    switch (format) {
                        case formatRGB24(): // 24bpp
                            rgbFormat = VideoFrameType::RGB24;
                            break;
                        case formatBGR24(): // 24bpp
                            rgbFormat = VideoFrameType::BGR24;
                            break;
                        default:
                            break;
                    }
                    if (rgbFormat.has_value()) {
                        return rtc::make_ref_counted<RGBBuffer>(IOSurfaceAutoRelease(buffer, retain),
                                                                rgbFormat.value(),
                                                                std::move(framesPool),
                                                                std::move(contentHint));
                    }
                }
                if (format == formatBGRA32()) {
                    return rtc::make_ref_counted<RGBBuffer>(IOSurfaceAutoRelease(buffer, retain),
                                                            VideoFrameType::BGRA32,
                                                            std::move(framesPool),
                                                            std::move(contentHint));
                }
                IOSurfaceUnlock(buffer, kIOSurfaceLockReadOnly, nullptr);
            }
        }
    }
    return nullptr;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> IOSurfaceBuffer::
    createFromSampleBuffer(CMSampleBufferRef buffer,
                           VideoFrameBufferPool framesPool,
                           std::optional<VideoContentHint> contentHint)
{
    CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(buffer);
    if (!pixelBuffer) {
        return {};
    }

    IOSurfaceRef ioSurface = CVPixelBufferGetIOSurface(pixelBuffer);
    if (!ioSurface) {
        return {};
    }

    CFArrayRef attachmentsArray = CMSampleBufferGetSampleAttachmentsArray(buffer, /*createIfNecessary=*/false);
    if (!attachmentsArray || CFArrayGetCount(attachmentsArray) <= 0) {
        return {};
    }
    return create(ioSurface, std::move(framesPool),
                  std::move(contentHint), true);
}

IOSurfaceRef IOSurfaceBuffer::pixelBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& videoPixelBuffer, bool retain)
{
    if (const auto accessor = dynamic_cast<const IOSBufferAccessor*>(videoPixelBuffer.get())) {
        return accessor->buffer(retain);
    }
    return nullptr;
}

} // namespace LiveKitCpp


namespace {

template<class TBaseVideoBuffer>
template <class... Args>
IOSBuffer<TBaseVideoBuffer>::IOSBuffer(IOSurfaceAutoRelease buffer,
                                       std::optional<VideoContentHint> contentHint,
                                       Args&&... args)
    : TBaseVideoBuffer(std::forward<Args>(args)...)
    , _buffer(std::move(buffer))
    , _contentHint(std::move(contentHint))
{
    IOSurfaceIncrementUseCount(_buffer.ref());
}

template<class TBaseVideoBuffer>
IOSBuffer<TBaseVideoBuffer>::~IOSBuffer()
{
    IOSurfaceUnlock(_buffer.ref(), kIOSurfaceLockReadOnly, nullptr);
    IOSurfaceDecrementUseCount(_buffer.ref());
}

template<class TBaseVideoBuffer>
size_t IOSBuffer<TBaseVideoBuffer>::surfaceStride(size_t planeIndex) const
{
    return IOSurfaceGetBytesPerRowOfPlane(_buffer.ref(), planeIndex);
}

template<class TBaseVideoBuffer>
const uint8_t* IOSBuffer<TBaseVideoBuffer>::surfaceData() const
{
    return reinterpret_cast<const uint8_t*>(IOSurfaceGetBaseAddress(_buffer.ref()));
}

template<class TBaseVideoBuffer>
const uint8_t* IOSBuffer<TBaseVideoBuffer>::surfaceData(size_t planeIndex) const
{
    return reinterpret_cast<const uint8_t*>(IOSurfaceGetBaseAddressOfPlane(_buffer.ref(), planeIndex));
}

template <class TBaseVideoBuffer>
VideoContentHint IOSBuffer<TBaseVideoBuffer>::contentHint() const
{
    if (_contentHint.has_value()) {
        return _contentHint.value();
    }
    return TBaseVideoBuffer::contentHint();
}

NV12Buffer::NV12Buffer(IOSurfaceAutoRelease buffer,
                       VideoFrameBufferPool framesPool,
                       std::optional<VideoContentHint> contentHint)
    : BaseClass(std::move(buffer), std::move(contentHint), std::move(framesPool))
{
}

std::string NV12Buffer::storage_representation() const
{
    return "LiveKitCpp::IOSurfaceNV12Buffer";
}

RGBBuffer::RGBBuffer(IOSurfaceAutoRelease buffer,
                     VideoFrameType rgbFormat,
                     VideoFrameBufferPool framesPool,
                     std::optional<VideoContentHint> contentHint)
    : BaseClass(std::move(buffer), std::move(contentHint),
                rgbFormat, std::move(framesPool))
{
}

std::string RGBBuffer::storage_representation() const
{
    return "LiveKitCpp::IOSurfaceRGBBuffer";
}

int RGBBuffer::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return static_cast<int>(surfaceStride(0U));
    }
    return 0;
}

const std::byte* RGBBuffer::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(surfaceData());
    }
    return nullptr;
}

} // namespace LiveKitCpp
