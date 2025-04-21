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
#pragma once // VideoFrameBuffer.h
#include "SafeScopedRefPtr.h"
#include <api/video/i420_buffer.h>
#include <api/video/nv12_buffer.h>
#include <type_traits>

namespace LiveKitCpp 
{

template <class TBaseBuffer = webrtc::VideoFrameBuffer>
class VideoFrameBuffer : public TBaseBuffer
{
    static_assert(std::is_base_of<webrtc::VideoFrameBuffer, TBaseBuffer>::value);
public:
    // impl. of webrtc::VideoFrameBuffer
    rtc::scoped_refptr<webrtc::VideoFrameBuffer>
        GetMappedFrameBuffer(rtc::ArrayView<webrtc::VideoFrameBuffer::Type> mappedTypes) override;
    rtc::scoped_refptr<webrtc::I420BufferInterface> ToI420() final;
protected:
    template <class... Args>
    VideoFrameBuffer(Args... args);
    virtual rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const = 0;
private:
    SafeScopedRefPtr<webrtc::I420BufferInterface> _i420;
};

template <class TBaseBuffer>
template <class... Args>
inline VideoFrameBuffer<TBaseBuffer>::VideoFrameBuffer(Args... args)
    : TBaseBuffer(std::forward<Args>(args)...)
{
}

template <class TBaseBuffer>
inline rtc::scoped_refptr<webrtc::VideoFrameBuffer> VideoFrameBuffer<TBaseBuffer>::
    GetMappedFrameBuffer(rtc::ArrayView<webrtc::VideoFrameBuffer::Type> mappedTypes)
{
    if (!mappedTypes.empty()) {
        for (const auto mappedType : mappedTypes) {
            switch (mappedType) {
                case webrtc::VideoFrameBuffer::Type::kI420:
                    return ToI420();
                default:
                    if (mappedType == this->type()) {
                        return rtc::scoped_refptr<webrtc::VideoFrameBuffer>(this);
                    }
                    break;
            }
        }
    }
    return nullptr;
}

template <class TBaseBuffer>
inline rtc::scoped_refptr<webrtc::I420BufferInterface> VideoFrameBuffer<TBaseBuffer>::ToI420()
{
    LOCK_WRITE_SAFE_OBJ(_i420);
    if (!_i420.constRef()) {
        _i420 = convertToI420();
    }
    return _i420.constRef();
}

} // namespace LiveKitCpp
