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
    std::shared_ptr<VideoFrame> impl;
    if (auto buffer = map(frame.video_frame_buffer())) {
        if (const auto type = detectType(buffer)) {
            const auto rotation = detectRotation(frame);
            impl.reset(new VideoFrameImpl(type.value(), rotation, std::move(buffer)));
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
            default:
                assert(false);
                break;
        }
    }
    return 0;
}

const void* VideoFrameImpl::data(size_t planeIndex) const
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
            default:
                assert(false);
                break;
        }
    }
    return nullptr;
}

std::optional<VideoFrameType> VideoFrameImpl::detectType(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer)
{
    if (buffer) {
        switch (buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kNative:
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

int VideoFrameImpl::detectRotation(const webrtc::VideoFrame& frame)
{
    switch (frame.rotation()) {
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
            case webrtc::VideoFrameBuffer::Type::kNative:
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

const void* VideoFrameImpl::data(size_t planeIndex, const webrtc::PlanarYuv8Buffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return buffer->DataY();
            case 1U:
                return buffer->DataU();
            case 2U:
                return buffer->DataV();
            default:
                break;
        }
    }
    return nullptr;
}

const void* VideoFrameImpl::data(size_t planeIndex, const webrtc::PlanarYuv16BBuffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return buffer->DataY();
            case 1U:
                return buffer->DataU();
            case 2U:
                return buffer->DataV();
            default:
                break;
        }
    }
    return nullptr;
}

const void* VideoFrameImpl::data(size_t planeIndex, const webrtc::BiplanarYuv8Buffer* buffer)
{
    if (buffer) {
        switch (planeIndex) {
            case 0U:
                return buffer->DataY();
            case 1U:
                return buffer->DataUV();
            default:
                break;
        }
    }
    return nullptr;
}

} // namespace LiveKitCpp
