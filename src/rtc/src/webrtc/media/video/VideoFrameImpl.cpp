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
#include "NativeVideoFrameBuffer.h"
#include <cassert>

namespace LiveKitCpp
{

VideoFrameImpl::VideoFrameImpl(VideoFrameType type, int rotation,
                               rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer)
    : VideoFrame(type, rotation)
    , _buffer(std::move(buffer))
{
}

std::shared_ptr<VideoFrame> VideoFrameImpl::create(const webrtc::VideoFrame& frame)
{
    return create(frame.video_frame_buffer(), frame.rotation());
}

std::shared_ptr<VideoFrame> VideoFrameImpl::create(rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                                                   webrtc::VideoRotation rotation)
{
    std::shared_ptr<VideoFrame> impl;
    buffer = map(std::move(buffer));
    if (buffer) {
        if (const auto type = detectType(buffer)) {
            impl.reset(new VideoFrameImpl(type.value(), map(rotation), std::move(buffer)));
        }
    }
    return impl;
}

std::shared_ptr<VideoFrame> VideoFrameImpl::convertToI420() const
{
    std::shared_ptr<VideoFrame> converted;
    if (_buffer) {
        if (auto i420 = _buffer->ToI420()) {
            assert(webrtc::VideoFrameBuffer::Type::kI420 == i420->type());
            converted.reset(new VideoFrameImpl(VideoFrameType::I420,
                                               rotation(),
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
                return dataSizeI420(planeIndex, _buffer->width(), _buffer->height());
            case webrtc::VideoFrameBuffer::Type::kI422:
                return dataSizeI422(planeIndex, _buffer->width(), _buffer->height());
            case webrtc::VideoFrameBuffer::Type::kI444:
                return dataSizeI444(planeIndex, _buffer->width(), _buffer->height());
            case webrtc::VideoFrameBuffer::Type::kI010:
                return dataSizeI010(planeIndex, _buffer->width(), _buffer->height());
            case webrtc::VideoFrameBuffer::Type::kI210:
                return dataSizeI210(planeIndex, _buffer->width(), _buffer->height());
            case webrtc::VideoFrameBuffer::Type::kI410:
                return dataSizeI410(planeIndex, _buffer->width(), _buffer->height());
            case webrtc::VideoFrameBuffer::Type::kNV12:
                return dataSizeNV12(planeIndex, _buffer->width(), _buffer->height());
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

std::optional<VideoFrameType> VideoFrameImpl::detectType(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer)
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

int VideoFrameImpl::map(webrtc::VideoRotation rotation)
{
    switch (rotation) {
        case webrtc::kVideoRotation_0:
            break;
        case webrtc::kVideoRotation_90:
            return 90;
        case webrtc::kVideoRotation_180:
            return 180;
            break;
        case webrtc::kVideoRotation_270:
            return 270;
        default:
            assert(false);
            break;
    }
    return 0;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> VideoFrameImpl::
    map(rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer)
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

int VideoFrameImpl::dataSizeI420(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
            return width * height;
        case 1U: // U
        case 2U: // V
            return (width / 2) * (height / 2);
        default:
            break;
    }
    return 0;
}

int VideoFrameImpl::dataSizeI422(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
            return width * height;
        case 1U: // U
        case 2U: // V
            return (width / 2) * height;
        default:
            break;
    }
    return 0;
}

int VideoFrameImpl::dataSizeI444(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
        case 1U: // U
        case 2U: // V
            return width * height;
        default:
            break;
    }
    return 0;
}

int VideoFrameImpl::dataSizeI010(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
            return width * height * 2;
        case 1U: // U
        case 2U: // V
            return (width / 2) * (height / 2) * 2;
        default:
            break;
    }
    return 0;
}

int VideoFrameImpl::dataSizeI210(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
            return width * height * 2;
        case 1U: // U
        case 2U: // V
            return (width / 2) * height * 2;
        default:
            break;
    }
    return 0;
}

int VideoFrameImpl::dataSizeI410(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
        case 1U: // U
        case 2U: // V
            return width * height * 2;
        default:
            break;
    }
    return 0;
}

int VideoFrameImpl::dataSizeNV12(size_t planeIndex, int width, int height)
{
    switch (planeIndex) {
        case 0U: // Y
            return width * height;
        case 1U: // UV
            return width * height / 2;
    }
    return 0;
}

VideoFrame::VideoFrame(VideoFrameType type, int rotation)
    : _type(type)
    , _rotation(rotation)
{
}

size_t VideoFrame::planesCount() const
{
    switch (type()) {
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
}

} // namespace LiveKitCpp
