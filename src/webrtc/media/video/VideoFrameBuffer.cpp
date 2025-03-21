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
#include "VideoFrameBuffer.h"
#include <rtc_base/time_utils.h>
#include <api/video/i010_buffer.h>
#include <api/video/i210_buffer.h>
#include <api/video/i410_buffer.h>
#include <api/video/i422_buffer.h>
#include <api/video/i444_buffer.h>

namespace LiveKitCpp
{

std::optional<webrtc::VideoFrame> createVideoFrame(int width, int height,
                                                   webrtc::VideoFrameBuffer::Type type,
                                                   int64_t timeStampMicro,
                                                   uint16_t id,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (width > 0 && height > 0) {
        if (webrtc::VideoFrameBuffer::Type::kNative == type ||
            webrtc::VideoFrameBuffer::Type::kI420A == type) {
            type = webrtc::VideoFrameBuffer::Type::kI420;
        }
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> buff;
        switch (type) {
            case webrtc::VideoFrameBuffer::Type::kI420:
                buff = webrtc::I420Buffer::Create(width, height);
                break;
            case webrtc::VideoFrameBuffer::Type::kI422:
                buff = webrtc::I422Buffer::Create(width, height);
                break;
            case webrtc::VideoFrameBuffer::Type::kI444:
                buff = webrtc::I444Buffer::Create(width, height);
                break;
            case webrtc::VideoFrameBuffer::Type::kI010:
                buff = webrtc::I010Buffer::Create(width, height);
                break;
            case webrtc::VideoFrameBuffer::Type::kI210:
                buff = webrtc::I210Buffer::Create(width, height);
                break;
            case webrtc::VideoFrameBuffer::Type::kI410:
                buff = webrtc::I410Buffer::Create(width, height);
                break;
            case webrtc::VideoFrameBuffer::Type::kNV12:
                buff = webrtc::NV12Buffer::Create(width, height);
                break;
            default:
                break;
        }
        return createVideoFrame(buff, timeStampMicro, id, colorSpace);
    }
    return std::nullopt;
}

std::optional<webrtc::VideoFrame> createVideoFrame(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                                                   int64_t timeStampMicro, uint16_t id,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (buff) {
        webrtc::VideoFrame::Builder builder;
        builder.set_video_frame_buffer(buff);
        if (timeStampMicro > 0LL) {
            builder.set_timestamp_us(timeStampMicro);
        } else {
            builder.set_timestamp_us(rtc::TimeMicros());
        }
        static thread_local uint16_t nextId = 1U;
        if (id > 0) {
            builder.set_id(id);
            nextId = id;
        } else {
            builder.set_id(nextId++);
        }
        auto frame = builder.build();
        frame.set_color_space(colorSpace);
        return frame;
    }
    return std::nullopt;
}

std::optional<webrtc::VideoFrame> createBlackVideoFrame(int width, int height,
                                                        int64_t timeStampMicro,
                                                        uint16_t id,
                                                        const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (width > 0 && height > 0) {
        if (const auto buff = webrtc::I420Buffer::Create(width, height)) {
            webrtc::I420Buffer::SetBlack(buff.get());
            return createVideoFrame(buff, timeStampMicro, id, colorSpace);
        }
    }
    return std::nullopt;
}


} // namespace LiveKitCpp
