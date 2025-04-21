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
#include "NV12VideoFrameBuffer.h"
#include "CoreVideoPixelBuffer.h"
#include <api/make_ref_counted.h>

namespace {

using namespace LiveKitCpp;

class IOSurfaceBufferAccessor
{
public:
    virtual IOSurfaceRef buffer(bool retain) const = 0;
protected:
    virtual ~IOSurfaceBufferAccessor() = default;
};

template<class TBaseVideoBuffer>
class IOSurfaceBufferHolder : public TBaseVideoBuffer, public IOSurfaceBufferAccessor
{
public:
    ~IOSurfaceBufferHolder();
    size_t surfaceWidth() const { return IOSurfaceGetWidth(_buffer.ref()); }
    size_t surfaceHeight() const { return IOSurfaceGetHeight(_buffer.ref()); }
    size_t surfaceStride() const { return IOSurfaceGetBytesPerRow(_buffer.ref()); }
    size_t surfaceStride(size_t planeIndex) const { return IOSurfaceGetBytesPerRowOfPlane(_buffer.ref(), planeIndex); }
    const uint8_t* surfaceData() const {
        return reinterpret_cast<const uint8_t*>(IOSurfaceGetBaseAddress(_buffer.ref()));
    }
    const uint8_t* surfaceData(size_t planeIndex) const {
        return reinterpret_cast<const uint8_t*>(IOSurfaceGetBaseAddressOfPlane(_buffer.ref(), planeIndex));
    }
    // impl. of IOSurfaceBufferAccessor
    IOSurfaceRef buffer(bool retain) const final { return _buffer.ref(retain); }
    
protected:
    template <class... AdditionalArgs>
    IOSurfaceBufferHolder(IOSurfaceRef buffer, bool retain, AdditionalArgs&&... additionalArgs);
    
private:
    const CFAutoRelease<IOSurfaceRef> _buffer;
};

class NV12IOSurfaceBuffer : public IOSurfaceBufferHolder<NV12VideoFrameBuffer>
{
    using BaseClass = IOSurfaceBufferHolder<NV12VideoFrameBuffer>;
public:
    NV12IOSurfaceBuffer(IOSurfaceRef buffer, bool retain);
    // impl. of webrtc::NV12BufferInterface
    const uint8_t* DataY() const final { return surfaceData(0U); }
    const uint8_t* DataUV() const final { return surfaceData(1U); }
    int StrideY() const final { return surfaceStride(0U); }
    int StrideUV() const final { return surfaceStride(1U); }
    int width() const final { return surfaceWidth(); }
    int height() const final { return surfaceHeight(); }
};

/*class RGBIOSurfaceBuffer : public IOSurfaceBufferHolder<VideoFrameRgbBuffer>
{
    using BaseClass = IOSurfaceBufferHolder<VideoFrameRgbBuffer>;
    
public:
    RGBIOSurfaceBuffer(IOSurfaceRef buffer, bool retain,
                       RgbFormat format,
                       const std::optional<ScaleMode>& scaleMode,
                       const std::weak_ptr<I420BuffersPool>& i420BuffersPoolRef);
    // impl. of VideoFrameRgbBuffer
    int width() const final { return surfaceWidth(); }
    int height() const final { return surfaceHeight(); }
    const uint8_t* data() const final { return surfaceData(); }
    int stride() const final { return surfaceStride(); }
};*/

}

namespace LiveKitCpp
{

bool IOSurfaceBuffer::supported(IOSurfaceRef buffer)
{
    return buffer && CoreVideoPixelBuffer::isSupportedFormat(IOSurfaceGetPixelFormat(buffer));
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> IOSurfaceBuffer::create(IOSurfaceRef buffer,
                                                                       bool retain)
{
    /*if (buffer) {
        const auto format = IOSurfaceGetPixelFormat(buffer);
        if (CoreVideoPixelBuffer::isSupportedFormat(format)) {
            if (kIOReturnSuccess == IOSurfaceLock(buffer, kIOSurfaceLockReadOnly, nullptr)) {
                if (CoreVideoPixelBuffer::isNV12Format(format)) {
                    return rtc::make_ref_counted<NV12IOSurfaceBuffer>(buffer, retain);
                }
                if (CoreVideoPixelBuffer::isRGB24Format(format)) {
                    return rtc::make_ref_counted<RGBIOSurfaceBuffer>(buffer, retain, RgbFormat::RGB24, scaleMode, i420BuffersPoolRef);
                }
                if (format == CoreVideoPixelBuffer::formatBGRA32()) {
                    return rtc::make_ref_counted<RGBIOSurfaceBuffer>(buffer, retain, RgbFormat::BGRA32, scaleMode, i420BuffersPoolRef);
                }
            }
        }
    }*/
    return nullptr;
}

IOSurfaceRef IOSurfaceBuffer::pixelBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& videoPixelBuffer, bool retain)
{
    if (const auto accessor = dynamic_cast<const IOSurfaceBufferAccessor*>(videoPixelBuffer.get())) {
        return accessor->buffer(retain);
    }
    return nullptr;
}

} // namespace darkmatter::rtc


namespace {

template<class TBaseVideoBuffer> template <class... AdditionalArgs>
IOSurfaceBufferHolder<TBaseVideoBuffer>::IOSurfaceBufferHolder(IOSurfaceRef buffer, bool retain,
                                                               AdditionalArgs&&... additionalArgs)
    : TBaseVideoBuffer(std::forward<AdditionalArgs>(additionalArgs)...)
    , _buffer(buffer, retain)
{
    IOSurfaceIncrementUseCount(_buffer.ref());
}

template<class TBaseVideoBuffer>
IOSurfaceBufferHolder<TBaseVideoBuffer>::~IOSurfaceBufferHolder()
{
    IOSurfaceUnlock(_buffer.ref(), kIOSurfaceLockReadOnly, nullptr);
    IOSurfaceDecrementUseCount(_buffer.ref());
}

NV12IOSurfaceBuffer::NV12IOSurfaceBuffer(IOSurfaceRef buffer, bool retain)
    : BaseClass(buffer, retain)
{
}

/*RGBIOSurfaceBuffer::RGBIOSurfaceBuffer(IOSurfaceRef buffer, bool retain,
                                       RgbFormat format,
                                       const std::optional<ScaleMode>& scaleMode,
                                       const std::weak_ptr<I420BuffersPool>& i420BuffersPoolRef)
    : BaseClass(buffer, retain, format, scaleMode, i420BuffersPoolRef)
{
}*/

} // namespace LiveKitCpp
