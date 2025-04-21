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
#include "VideoUtils.h"
#include "CapturerState.h"
#include <api/video/i420_buffer.h>
#include "Utils.h"
#include "livekit/rtc/media/VideoOptions.h"
#include <rtc_base/time_utils.h>
#include <api/video/i420_buffer.h>
#include <api/video/nv12_buffer.h>
#include <api/video/i010_buffer.h>
#include <api/video/i210_buffer.h>
#include <api/video/i410_buffer.h>
#include <api/video/i422_buffer.h>
#include <api/video/i444_buffer.h>
#include <cassert>

namespace
{

inline constexpr unsigned g_interlaced  = 0x0001;
inline constexpr unsigned g_previewMode = 0x0002;

}

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

webrtc::VideoTrackInterface::ContentHint map(VideoContentHint hint)
{
    switch (hint) {
        case VideoContentHint::None:
            break;
        case VideoContentHint::Fluid:
            return webrtc::VideoTrackInterface::ContentHint::kFluid;
        case VideoContentHint::Detailed:
            return webrtc::VideoTrackInterface::ContentHint::kDetailed;
        case VideoContentHint::Text:
            return webrtc::VideoTrackInterface::ContentHint::kText;
        default:
            assert(false);
            break;
    }
    return webrtc::VideoTrackInterface::ContentHint::kNone;
}

VideoContentHint map(webrtc::VideoTrackInterface::ContentHint hint)
{
    switch (hint) {
        case webrtc::VideoTrackInterface::ContentHint::kNone:
            break;
        case webrtc::VideoTrackInterface::ContentHint::kFluid:
            return VideoContentHint::Fluid;
        case webrtc::VideoTrackInterface::ContentHint::kDetailed:
            return VideoContentHint::Detailed;
        case webrtc::VideoTrackInterface::ContentHint::kText:
            return VideoContentHint::Text;
        default:
            assert(false);
            break;
    }
    return VideoContentHint::None;
}

void VideoOptions::setInterlaced(bool interlaced)
{
    if (interlaced) {
        _flags |= g_interlaced;
    }
    else {
        _flags &= ~g_interlaced;
    }
}

void VideoOptions::setPreview(bool interlaced)
{
    if (interlaced) {
        _flags |= g_previewMode;
    }
    else {
        _flags &= ~g_previewMode;
    }
}

bool VideoOptions::interlaced() const noexcept
{
    return testFlag<g_interlaced>(_flags);
}

bool VideoOptions::preview() const noexcept
{
    return testFlag<g_previewMode>(_flags);
}

bool acceptState(CapturerState currentState, CapturerState newState)
{
    switch (currentState) {
        case CapturerState::Stopping:
            return true;
        case CapturerState::Stopped:
            return CapturerState::Starting == newState || CapturerState::Started == newState;
        case CapturerState::Starting: // any state is good
            return true;
        case CapturerState::Started:
            return CapturerState::Stopping == newState || CapturerState::Stopped == newState;
        default:
            assert(false);
            break;
    }
    return false;
}

#ifdef WEBRTC_MAC
bool isNV12Format(OSType format)
{
    switch (format) {
        case formatNV12Full():
        case formatNV12Video():
            return true;
        default:
            break;
    }
    return false;
}

bool isRGB24Format(OSType format)
{
    switch (format) {
        case formatRGB24():
        case formatBGR24():
            return true;
        default:
            break;
    }
    return false;
}

bool isRGB32Format(OSType format)
{
    switch (format) {
        case formatBGRA32():
        case formatARGB32():
        case formatRGBA32():
            return true;
        default:
            break;
    }
    return false;
}

bool isSupportedFormat(OSType format)
{
    return isNV12Format(format) || isRGBFormat(format);
}
#endif

} // namespace LiveKitCpp
