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
#include "VideoFrameImpl.h"
#include "LibyuvImport.h"
#include "RgbGenericVideoFrameBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "VideoUtils.h"
#include "VideoFrameBuffer.h"
#include <rtc_base/time_utils.h>
#include <cassert>

namespace
{

using namespace LiveKitCpp;

template <class TBaseBuffer>
class ExternalFrameHolder : public TBaseBuffer
{
    static_assert(std::is_base_of<webrtc::VideoFrameBuffer, TBaseBuffer>::value);
public:
    // impl. of webrtc::VideoFrameBuffer
    int width() const final { return _frame->width(); }
    int height() const final { return _frame->height(); }
protected:
    template <class... Args>
    ExternalFrameHolder(const std::shared_ptr<VideoFrame>& frame, Args... args);
    const auto& frame() const noexcept { return _frame; }
    int stride(size_t planeIndex) const { return _frame->stride(planeIndex);}
    int dataSize(size_t planeIndex) const { return _frame->dataSize(planeIndex);}
    template <typename T = uint8_t>
    const T* data(size_t planeIndex) const { return reinterpret_cast<const T*>(_frame->data(planeIndex)); }
private:
    const std::shared_ptr<VideoFrame> _frame;
};

template <class TBaseBuffer>
class ExternalVideoFrameBuffer : public ExternalFrameHolder<VideoFrameBuffer<TBaseBuffer>>
{
protected:
    ExternalVideoFrameBuffer(const std::shared_ptr<VideoFrame>& frame,
                             VideoFrameBufferPool framesPool = {});
};

class ExternalMJpegBuffer : public ExternalVideoFrameBuffer<NativeVideoFrameBuffer>
{
    using Base = ExternalVideoFrameBuffer<NativeVideoFrameBuffer>;
public:
    ExternalMJpegBuffer(const std::shared_ptr<VideoFrame>& frame,
                        VideoFrameBufferPool framesPool = {});
    // impl. of NativeVideoFrameBuffer
    VideoFrameType nativeType() const final { return VideoFrameType::MJPEG; }
    int stride(size_t planeIndex) const final { return frame()->stride(planeIndex); }
    const std::byte* data(size_t planeIndex) const final { return frame()->data(planeIndex); }
    int dataSize(size_t planeIndex) const final { return frame()->dataSize(planeIndex); }
private:
    // impl. of VideoFrameBuffer<>
    webrtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

class ExternalRgbBuffer : public ExternalFrameHolder<RgbGenericVideoFrameBuffer>
{
    using Base = ExternalFrameHolder<RgbGenericVideoFrameBuffer>;
public:
    ExternalRgbBuffer(const std::shared_ptr<VideoFrame>& frame,
                      VideoFrameBufferPool framesPool = {});
    // impl. of NativeVideoFrameBuffer
    int stride(size_t planeIndex) const final { return Base::stride(planeIndex); }
    const std::byte* data(size_t planeIndex) const final { return Base::data<std::byte>(planeIndex); }
    int dataSize(size_t planeIndex) const final { return Base::dataSize(planeIndex); }
};

class ExternalNV12Buffer : public ExternalFrameHolder<NV12VideoFrameBuffer>
{
public:
    ExternalNV12Buffer(const std::shared_ptr<VideoFrame>& frame,
                       VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::BiplanarYuv8Buffer
    const uint8_t* DataY() const final { return data(0U); }
    const uint8_t* DataUV() const final { return data(1U); }
    // impl. of webrtc::BiplanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideUV() const final { return stride(1U); }
};

class ExternalI420Buffer : public ExternalFrameHolder<webrtc::I420BufferInterface>
{
public:
    ExternalI420Buffer(const std::shared_ptr<VideoFrame>& frame);
    // impl. of webrtc::PlanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideU() const final { return stride(1U); }
    int StrideV() const final { return stride(2U); }
    // impl. of PlanarYuv8Buffer
    const uint8_t* DataY() const final { return data(0U); }
    const uint8_t* DataU() const final { return data(1U); }
    const uint8_t* DataV() const final { return data(2U); }
};

class ExternalI422Buffer : public ExternalVideoFrameBuffer<webrtc::I422BufferInterface>
{
public:
    ExternalI422Buffer(const std::shared_ptr<VideoFrame>& frame,
                       VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::PlanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideU() const final { return stride(1U); }
    int StrideV() const final { return stride(2U); }
    // impl. of PlanarYuv8Buffer
    const uint8_t* DataY() const final { return data(0U); }
    const uint8_t* DataU() const final { return data(1U); }
    const uint8_t* DataV() const final { return data(2U); }
private:
    // impl. of VideoFrameBuffer<>
    webrtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

class ExternalI444Buffer : public ExternalVideoFrameBuffer<webrtc::I444BufferInterface>
{
public:
    ExternalI444Buffer(const std::shared_ptr<VideoFrame>& frame,
                       VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::PlanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideU() const final { return stride(1U); }
    int StrideV() const final { return stride(2U); }
    // impl. of PlanarYuv8Buffer
    const uint8_t* DataY() const final { return data(0U); }
    const uint8_t* DataU() const final { return data(1U); }
    const uint8_t* DataV() const final { return data(2U); }
private:
    // impl. of VideoFrameBuffer<>
    webrtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

class ExternalI010Buffer : public ExternalVideoFrameBuffer<webrtc::I010BufferInterface>
{
public:
    ExternalI010Buffer(const std::shared_ptr<VideoFrame>& frame,
                       VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::PlanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideU() const final { return stride(1U); }
    int StrideV() const final { return stride(2U); }
    // impl. of PlanarYuv8Buffer
    const uint16_t* DataY() const final { return data<uint16_t>(0U); }
    const uint16_t* DataU() const final { return data<uint16_t>(1U); }
    const uint16_t* DataV() const final { return data<uint16_t>(2U); }
private:
    // impl. of VideoFrameBuffer<>
    webrtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

class ExternalI210Buffer : public ExternalVideoFrameBuffer<webrtc::I210BufferInterface>
{
public:
    ExternalI210Buffer(const std::shared_ptr<VideoFrame>& frame,
                       VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::PlanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideU() const final { return stride(1U); }
    int StrideV() const final { return stride(2U); }
    // impl. of PlanarYuv8Buffer
    const uint16_t* DataY() const final { return data<uint16_t>(0U); }
    const uint16_t* DataU() const final { return data<uint16_t>(1U); }
    const uint16_t* DataV() const final { return data<uint16_t>(2U); }
private:
    // impl. of VideoFrameBuffer<>
    webrtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

class ExternalI410Buffer : public ExternalVideoFrameBuffer<webrtc::I410BufferInterface>
{
public:
    ExternalI410Buffer(const std::shared_ptr<VideoFrame>& frame,
                       VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::PlanarYuvBuffer
    int StrideY() const final { return stride(0U); }
    int StrideU() const final { return stride(1U); }
    int StrideV() const final { return stride(2U); }
    // impl. of PlanarYuv8Buffer
    const uint16_t* DataY() const final { return data<uint16_t>(0U); }
    const uint16_t* DataU() const final { return data<uint16_t>(1U); }
    const uint16_t* DataV() const final { return data<uint16_t>(2U); }
private:
    // impl. of VideoFrameBuffer<>
    webrtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
};

inline int halfHeight(const webrtc::VideoFrameBuffer* buffer) {
    if (buffer) {
        return (buffer->height() + 1) / 2;
    }
    return 0;
}

}

namespace LiveKitCpp
{

VideoFrameImpl::VideoFrameImpl(VideoFrameType type, int rotation, int64_t timestampUs,
                               webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer)
    : VideoFrame(type, rotation, timestampUs)
    , _buffer(std::move(buffer))
{
}

std::shared_ptr<VideoFrame> VideoFrameImpl::create(const webrtc::VideoFrame& frame)
{
    return create(frame.video_frame_buffer(), frame.rotation(), frame.timestamp_us());
}

std::shared_ptr<VideoFrame> VideoFrameImpl::create(webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                                                   webrtc::VideoRotation rotation,
                                                   int64_t timestampUs)
{
    std::shared_ptr<VideoFrame> impl;
    buffer = map(std::move(buffer));
    if (buffer) {
        if (const auto type = detectType(buffer)) {
            impl.reset(new VideoFrameImpl(type.value(), mapFromRtc(rotation), timestampUs, std::move(buffer)));
        }
    }
    return impl;
}

std::optional<webrtc::VideoFrame> VideoFrameImpl::create(const std::shared_ptr<VideoFrame>& frame,
                                                         VideoFrameBufferPool framesPool)
{
    if (frame) {
        webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer;
        if (const auto impl = std::dynamic_pointer_cast<VideoFrameImpl>(frame)) {
            buffer = impl->_buffer;
        }
        else {
            if (isRGB(frame->type())) {
                buffer = webrtc::make_ref_counted<ExternalRgbBuffer>(frame, std::move(framesPool));
            }
            else {
                switch (frame->type()) {
                    case VideoFrameType::NV12:
                        buffer = webrtc::make_ref_counted<ExternalNV12Buffer>(frame, std::move(framesPool));
                        break;
                    case VideoFrameType::I420:
                        buffer = webrtc::make_ref_counted<ExternalI420Buffer>(frame);
                        break;
                    case VideoFrameType::I422:
                        buffer = webrtc::make_ref_counted<ExternalI422Buffer>(frame, std::move(framesPool));
                        break;
                    case VideoFrameType::I444:
                        buffer = webrtc::make_ref_counted<ExternalI444Buffer>(frame, std::move(framesPool));
                        break;
                    case VideoFrameType::I010:
                        buffer = webrtc::make_ref_counted<ExternalI010Buffer>(frame, std::move(framesPool));
                        break;
                    case VideoFrameType::I210:
                        buffer = webrtc::make_ref_counted<ExternalI210Buffer>(frame, std::move(framesPool));
                        break;
                    case VideoFrameType::I410:
                        buffer = webrtc::make_ref_counted<ExternalI410Buffer>(frame, std::move(framesPool));
                        break;
                    case VideoFrameType::MJPEG:
                        buffer = webrtc::make_ref_counted<ExternalMJpegBuffer>(frame, std::move(framesPool));
                        break;
                    default:
                        break;
                }
            }
        }
        if (buffer) {
            return createVideoFrame(buffer, mapToRtc(frame->rotation()), frame->timestampUs());
        }
    }
    return std::nullopt;
}

std::shared_ptr<VideoFrame> VideoFrameImpl::convertToI420() const
{
    std::shared_ptr<VideoFrame> converted;
    if (_buffer) {
        if (auto i420 = _buffer->ToI420()) {
            assert(webrtc::VideoFrameBuffer::Type::kI420 == i420->type());
            converted.reset(new VideoFrameImpl(VideoFrameType::I420,
                                               rotation(),
                                               timestampUs(),
                                               std::move(i420)));
        }
    }
    return converted;
}

int VideoFrameImpl::width() const
{
    return _buffer ? _buffer->width() : 0;
}

int VideoFrameImpl::height() const
{
    return _buffer ? _buffer->height() : 0;
}

int VideoFrameImpl::stride(size_t planeIndex) const
{
    if (_buffer) {
        switch (_buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kI420:
                return stride(planeIndex, _buffer->GetI420());
            case webrtc::VideoFrameBuffer::Type::kI422:
                return stride(planeIndex, _buffer->GetI422());
            case webrtc::VideoFrameBuffer::Type::kI444:
                return stride(planeIndex, _buffer->GetI444());
            case webrtc::VideoFrameBuffer::Type::kI010:
                return stride(planeIndex, _buffer->GetI010());
            case webrtc::VideoFrameBuffer::Type::kI210:
                return stride(planeIndex, _buffer->GetI210());
            case webrtc::VideoFrameBuffer::Type::kI410:
                return stride(planeIndex, _buffer->GetI410());
            case webrtc::VideoFrameBuffer::Type::kNV12:
                return stride(planeIndex, _buffer->GetNV12());
            case webrtc::VideoFrameBuffer::Type::kNative:
                if (const auto native = dynamic_cast<const NativeVideoFrameBuffer*>(_buffer.get())) {
                    return native->stride(planeIndex);
                }
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

const std::byte* VideoFrameImpl::data(size_t planeIndex) const
{
    if (_buffer) {
        switch (_buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kI420:
                return data(planeIndex, _buffer->GetI420());
            case webrtc::VideoFrameBuffer::Type::kI422:
                return data(planeIndex, _buffer->GetI422());
            case webrtc::VideoFrameBuffer::Type::kI444:
                return data(planeIndex, _buffer->GetI444());
            case webrtc::VideoFrameBuffer::Type::kI010:
                return data(planeIndex, _buffer->GetI010());
            case webrtc::VideoFrameBuffer::Type::kI210:
                return data(planeIndex, _buffer->GetI210());
            case webrtc::VideoFrameBuffer::Type::kI410:
                return data(planeIndex, _buffer->GetI410());
            case webrtc::VideoFrameBuffer::Type::kNV12:
                return data(planeIndex, _buffer->GetNV12());
            case webrtc::VideoFrameBuffer::Type::kNative:
                if (const auto native = dynamic_cast<const NativeVideoFrameBuffer*>(_buffer.get())) {
                    return native->data(planeIndex);
                }
            default:
                assert(false);
                break;
        }
    }
    return nullptr;
}

int VideoFrameImpl::dataSize(size_t planeIndex) const
{
    if (_buffer) {
        switch (_buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kI420:
                return dataSizeI420(planeIndex, _buffer->GetI420());
            case webrtc::VideoFrameBuffer::Type::kI422:
                return dataSizeI422(planeIndex, _buffer->GetI422());
            case webrtc::VideoFrameBuffer::Type::kI444:
                return dataSizeI444(planeIndex, _buffer->GetI444());
            case webrtc::VideoFrameBuffer::Type::kI010:
                return dataSizeI010(planeIndex, _buffer->GetI010());
            case webrtc::VideoFrameBuffer::Type::kI210:
                return dataSizeI210(planeIndex, _buffer->GetI210());
            case webrtc::VideoFrameBuffer::Type::kI410:
                return dataSizeI410(planeIndex, _buffer->GetI410());
            case webrtc::VideoFrameBuffer::Type::kNV12:
                return dataSizeNV12(planeIndex, _buffer->GetNV12());
            case webrtc::VideoFrameBuffer::Type::kNative:
                if (const auto native = dynamic_cast<const NativeVideoFrameBuffer*>(_buffer.get())) {
                    return native->dataSize(planeIndex);
                }
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

std::optional<VideoFrameType> VideoFrameImpl::detectType(const webrtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer)
{
    if (buffer) {
        switch (buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kNative:
                if (const auto native = dynamic_cast<const NativeVideoFrameBuffer*>(buffer.get())) {
                    return native->nativeType();
                }
                break;
            case webrtc::VideoFrameBuffer::Type::kI420A:
                assert(false);
            case webrtc::VideoFrameBuffer::Type::kI420:
                return VideoFrameType::I420;
            case webrtc::VideoFrameBuffer::Type::kI422:
                return VideoFrameType::I422;
            case webrtc::VideoFrameBuffer::Type::kI444:
                return VideoFrameType::I444;
            case webrtc::VideoFrameBuffer::Type::kI010:
                return VideoFrameType::I010;
            case webrtc::VideoFrameBuffer::Type::kI210:
                return VideoFrameType::I210;
            case webrtc::VideoFrameBuffer::Type::kI410:
                return VideoFrameType::I410;
            case webrtc::VideoFrameBuffer::Type::kNV12:
                return VideoFrameType::NV12;
            default:
                assert(false);
                break;
        }
    }
    return std::nullopt;
}

int VideoFrameImpl::mapFromRtc(webrtc::VideoRotation rotation)
{
    switch (rotation) {
        case webrtc::kVideoRotation_0:
            break;
        case webrtc::kVideoRotation_90:
            return 90;
        case webrtc::kVideoRotation_180:
            return 180;
        case webrtc::kVideoRotation_270:
            return 270;
        default:
            assert(false);
            break;
    }
    return 0;
}

webrtc::VideoRotation VideoFrameImpl::mapToRtc(int rotation)
{
    switch (rotation) {
        case 0:
            break;
        case 90:
            return webrtc::kVideoRotation_90;
        case 180:
            return webrtc::kVideoRotation_180;
        case 270:
            return webrtc::kVideoRotation_270;
        default:
            assert(false);
            break;
    }
    return webrtc::kVideoRotation_0;
}

webrtc::scoped_refptr<webrtc::VideoFrameBuffer> VideoFrameImpl::
    map(webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer)
{
    if (buffer) {
        switch (buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kI420A:
                return buffer->ToI420();
            case webrtc::VideoFrameBuffer::Type::kNative:
                if (dynamic_cast<const NativeVideoFrameBuffer*>(buffer.get())) {
                    return buffer;
                }
                return buffer->ToI420();
            default:
                break;
        }
    }
    return buffer;
}

int VideoFrameImpl::stride(size_t planeIndex, const webrtc::PlanarYuvBuffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return buffer->StrideY();
            case 1U:
                return buffer->StrideU();
            case 2U:
                return buffer->StrideV();
            default:
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::stride(size_t planeIndex, const webrtc::BiplanarYuvBuffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return buffer->StrideY();
            case 1U:
                return buffer->StrideUV();
            default:
                break;
        }
    }
    return 0;
}

const std::byte* VideoFrameImpl::data(size_t planeIndex, const webrtc::PlanarYuv8Buffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return reinterpret_cast<const std::byte*>(buffer->DataY());
            case 1U:
                return reinterpret_cast<const std::byte*>(buffer->DataU());
            case 2U:
                return reinterpret_cast<const std::byte*>(buffer->DataV());
            default:
                break;
        }
    }
    return nullptr;
}

const std::byte* VideoFrameImpl::data(size_t planeIndex, const webrtc::PlanarYuv16BBuffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return reinterpret_cast<const std::byte*>(buffer->DataY());
            case 1U:
                return reinterpret_cast<const std::byte*>(buffer->DataU());
            case 2U:
                return reinterpret_cast<const std::byte*>(buffer->DataV());
            default:
                break;
        }
    }
    return nullptr;
}

const std::byte* VideoFrameImpl::data(size_t planeIndex, const webrtc::BiplanarYuv8Buffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return reinterpret_cast<const std::byte*>(buffer->DataY());
            case 1U:
                return reinterpret_cast<const std::byte*>(buffer->DataUV());
            default:
                break;
        }
    }
    return nullptr;
}

int VideoFrameImpl::dataSizeI420(size_t planeIndex, const webrtc::I420BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/i420_buffer.cc#46
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height();
            case 1U: // U
                return buffer->StrideU() * halfHeight(buffer);
            case 2U: // V
                return buffer->StrideV() * halfHeight(buffer);
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::dataSizeI422(size_t planeIndex, const webrtc::I422BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/i422_buffer.cc#46
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height();
            case 1U: // U
                return buffer->StrideU() * buffer->height();
            case 2U: // V
                return buffer->StrideV() * buffer->height();
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::dataSizeI444(size_t planeIndex, const webrtc::I444BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/i444_buffer.cc#45
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height();
            case 1U: // U
                return buffer->StrideU() * buffer->height();
            case 2U: // V
                return buffer->StrideV() * buffer->height();
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::dataSizeI010(size_t planeIndex, const webrtc::I010BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/i010_buffer.cc#43
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height() * 2;
            case 1U: // U
                return buffer->StrideU() * halfHeight(buffer) * 2;
            case 2U: // V
                return buffer->StrideV() * halfHeight(buffer) * 2;
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::dataSizeI210(size_t planeIndex, const webrtc::I210BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/i210_buffer.cc#44
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height() * 2;
            case 1U: // U
                return buffer->StrideU() * buffer->height() * 2;
            case 2U: // V
                return buffer->StrideV() * buffer->height() * 2;
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::dataSizeI410(size_t planeIndex, const webrtc::I410BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/i410_buffer.cc#46
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height() * 2;
            case 1U: // U
                return buffer->StrideU() * buffer->height() * 2;
            case 2U: // V
                return buffer->StrideV() * buffer->height() * 2;
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

int VideoFrameImpl::dataSizeNV12(size_t planeIndex, const webrtc::NV12BufferInterface* buffer)
{
    if (buffer) {
        // https://webrtc.googlesource.com/src/+/refs/heads/main/api/video/nv12_buffer.cc#37
        switch (planeIndex) {
            case 0U: // Y
                return buffer->StrideY() * buffer->height();
            case 1U: // UV
                return buffer->StrideUV() * halfHeight(buffer);
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

VideoFrame::VideoFrame(VideoFrameType type, int rotation, int64_t timestampUs)
    : _type(type)
    , _rotation(rotation)
    , _timestampUs(timestampUs ? timestampUs : webrtc::TimeMicros())
{
}

size_t VideoFrame::planesCount() const
{
    return LiveKitCpp::planesCount(type());
}

std::string toString(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::RGB24:
            return "RGB24";
        case VideoFrameType::BGR24:
            return "BGR24";
        case VideoFrameType::BGRA32:
            return "BGRA32";
        case VideoFrameType::ARGB32:
            return "ARGB32";
        case VideoFrameType::RGBA32:
            return "RGBA32";
        case VideoFrameType::ABGR32:
            return "ABGR32";
        case VideoFrameType::RGB565:
            return "ABGR32";
        case VideoFrameType::MJPEG:
            return "MJPEG";
        case VideoFrameType::UYVY:
            return "UYVY";
        case VideoFrameType::YUY2:
            return "YUY2";
        case VideoFrameType::NV12:
            return "NV12";
        case VideoFrameType::I420:
            return "I420";
        case VideoFrameType::I422:
            return "I422";
        case VideoFrameType::I444:
            return "I444";
        case VideoFrameType::I010:
            return "I010";
        case VideoFrameType::I210:
            return "I210";
        case VideoFrameType::I410:
            return "I410";
        case VideoFrameType::YV12:
            return "YV12";
        case VideoFrameType::IYUV:
            return "IYUV";
        default:
            assert(false);
            break;
    }
    return {};
}

bool isRGB(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::RGB24:
        case VideoFrameType::BGR24:
        case VideoFrameType::BGRA32:
        case VideoFrameType::ARGB32:
        case VideoFrameType::RGBA32:
        case VideoFrameType::ABGR32:
        case VideoFrameType::RGB565:
            return true;
        default:
            break;
    }
    return false;
}

size_t planesCount(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::RGB24:
        case VideoFrameType::BGR24:
        case VideoFrameType::BGRA32:
        case VideoFrameType::ARGB32:
        case VideoFrameType::RGBA32:
        case VideoFrameType::ABGR32:
        case VideoFrameType::RGB565:
        case VideoFrameType::MJPEG:
        case VideoFrameType::UYVY:
        case VideoFrameType::YUY2:
            return 1U;
        case VideoFrameType::NV12:
            return 2U;
        case VideoFrameType::I420:
        case VideoFrameType::I422:
        case VideoFrameType::I444:
        case VideoFrameType::I010:
        case VideoFrameType::I210:
        case VideoFrameType::I410:
        case VideoFrameType::YV12:
        case VideoFrameType::IYUV:
            return 3U;
        default:
            assert(false);
            break;
    }
    return 0U;
}

} // namespace LiveKitCpp

namespace
{


template <class TBaseBuffer>
template <class... Args>
ExternalFrameHolder<TBaseBuffer>::ExternalFrameHolder(const std::shared_ptr<VideoFrame>& frame, Args... args)
    : TBaseBuffer(std::forward<Args>(args)...)
    , _frame(frame)
{
}

template <class TBaseBuffer>
ExternalVideoFrameBuffer<TBaseBuffer>::ExternalVideoFrameBuffer(const std::shared_ptr<VideoFrame>& frame,
                                                                VideoFrameBufferPool framesPool)
    : ExternalFrameHolder<VideoFrameBuffer<TBaseBuffer>>(frame, std::move(framesPool))
{
}

ExternalMJpegBuffer::ExternalMJpegBuffer(const std::shared_ptr<VideoFrame>& frame,
                                           VideoFrameBufferPool framesPool)
    : Base(frame, std::move(framesPool))
{
}

webrtc::scoped_refptr<webrtc::I420BufferInterface> ExternalMJpegBuffer::convertToI420() const
{
    auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::MJPGToI420(Base::data<uint8_t>(0U), dataSize(0),
                                        i420->MutableDataY(), i420->StrideY(),
                                        i420->MutableDataU(), i420->StrideU(),
                                        i420->MutableDataV(), i420->StrideV(),
                                        width(), height(),
                                        i420->width(), i420->height())) {
        return i420;
    }
    return {};
}

ExternalRgbBuffer::ExternalRgbBuffer(const std::shared_ptr<VideoFrame>& frame,
                                     VideoFrameBufferPool framesPool)
    : Base(frame, frame->type(), std::move(framesPool))
{
}

ExternalNV12Buffer::ExternalNV12Buffer(const std::shared_ptr<VideoFrame>& frame,
                                       VideoFrameBufferPool framesPool)
    : ExternalFrameHolder<NV12VideoFrameBuffer>(frame, std::move(framesPool))
{
}

ExternalI420Buffer::ExternalI420Buffer(const std::shared_ptr<VideoFrame>& frame)
    : ExternalFrameHolder<webrtc::I420BufferInterface>(frame)
{
}

ExternalI422Buffer::ExternalI422Buffer(const std::shared_ptr<VideoFrame>& frame,
                                       VideoFrameBufferPool framesPool)
    : ExternalVideoFrameBuffer<webrtc::I422BufferInterface>(frame, std::move(framesPool))
{
}

webrtc::scoped_refptr<webrtc::I420BufferInterface> ExternalI422Buffer::convertToI420() const
{
    auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::I422ToI420(DataY(), StrideY(),
                                        DataU(), StrideU(),
                                        DataV(), StrideV(),
                                        i420->MutableDataY(), i420->StrideY(),
                                        i420->MutableDataU(), i420->StrideU(),
                                        i420->MutableDataV(), i420->StrideV(),
                                        width(), height())) {
        return i420;
    }
    return {};
}

ExternalI444Buffer::ExternalI444Buffer(const std::shared_ptr<VideoFrame>& frame,
                                       VideoFrameBufferPool framesPool)
    : ExternalVideoFrameBuffer<webrtc::I444BufferInterface>(frame, std::move(framesPool))
{
}

webrtc::scoped_refptr<webrtc::I420BufferInterface> ExternalI444Buffer::convertToI420() const
{
    auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::I444ToI420(DataY(), StrideY(),
                                        DataU(), StrideU(),
                                        DataV(), StrideV(),
                                        i420->MutableDataY(), i420->StrideY(),
                                        i420->MutableDataU(), i420->StrideU(),
                                        i420->MutableDataV(), i420->StrideV(),
                                        width(), height())) {
        return i420;
    }
    return {};
}

ExternalI010Buffer::ExternalI010Buffer(const std::shared_ptr<VideoFrame>& frame,
                                       VideoFrameBufferPool framesPool)
    : ExternalVideoFrameBuffer<webrtc::I010BufferInterface>(frame, std::move(framesPool))
{
}

webrtc::scoped_refptr<webrtc::I420BufferInterface> ExternalI010Buffer::convertToI420() const
{
    auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::I010ToI420(DataY(), StrideY(),
                                        DataU(), StrideU(),
                                        DataV(), StrideV(),
                                        i420->MutableDataY(), i420->StrideY(),
                                        i420->MutableDataU(), i420->StrideU(),
                                        i420->MutableDataV(), i420->StrideV(),
                                        width(), height())) {
        return i420;
    }
    return {};
}

ExternalI210Buffer::ExternalI210Buffer(const std::shared_ptr<VideoFrame>& frame,
                                       VideoFrameBufferPool framesPool)
    : ExternalVideoFrameBuffer<webrtc::I210BufferInterface>(frame, std::move(framesPool))
{
}

webrtc::scoped_refptr<webrtc::I420BufferInterface> ExternalI210Buffer::convertToI420() const
{
    auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::I210ToI420(DataY(), StrideY(),
                                        DataU(), StrideU(),
                                        DataV(), StrideV(),
                                        i420->MutableDataY(), i420->StrideY(),
                                        i420->MutableDataU(), i420->StrideU(),
                                        i420->MutableDataV(), i420->StrideV(),
                                        width(), height())) {
        return i420;
    }
    return {};
}

ExternalI410Buffer::ExternalI410Buffer(const std::shared_ptr<VideoFrame>& frame,
                                       VideoFrameBufferPool framesPool)
    : ExternalVideoFrameBuffer<webrtc::I410BufferInterface>(frame, std::move(framesPool))
{
}

webrtc::scoped_refptr<webrtc::I420BufferInterface> ExternalI410Buffer::convertToI420() const
{
    auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::I410ToI420(DataY(), StrideY(),
                                        DataU(), StrideU(),
                                        DataV(), StrideV(),
                                        i420->MutableDataY(), i420->StrideY(),
                                        i420->MutableDataU(), i420->StrideU(),
                                        i420->MutableDataV(), i420->StrideV(),
                                        width(), height())) {
        return i420;
    }
    return {};
}

}
