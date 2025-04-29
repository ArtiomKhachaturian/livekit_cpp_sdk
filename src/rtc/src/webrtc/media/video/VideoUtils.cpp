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
#include "LibyuvImport.h"
/*#ifdef WEBRTC_MAC
#include "MetalRGBScaler.h"
#endif*/
#include "livekit/rtc/media/VideoOptions.h"
#include <rtc_base/time_utils.h>
#include <cassert>

namespace
{

using namespace LiveKitCpp;

bool cpuScaleNV12(const uint8_t* srcY, int srcStrideY,
                  const uint8_t* srcUV, int srcStrideUV,
                  int srcWidth, int srcHeight,
                  uint8_t* dstY, int dstStrideY,
                  uint8_t* dstUV, int dstStrideUV,
                  int dstWidth, int dstHeight,
                  VideoContentHint hint);

bool cpuScaleRGB24(const std::byte* srcRGB, int srcStrideRGB,
                   int srcWidth, int srcHeight,
                   std::byte* dstRGB, int dstStrideRGB,
                   int dstWidth, int dstHeight,
                   VideoContentHint hint);

bool cpuScaleRGB32(const std::byte* srcARGB, int srcStrideARGB,
                   int srcWidth, int srcHeight,
                   std::byte* dstARGB, int dstStrideARGB,
                   int dstWidth, int dstHeight,
                   VideoContentHint hint);

bool gpuScaleRGB(const std::byte* src, int srcStride,
                 int srcWidth, int srcHeight,
                 std::byte* dst, int dstStride,
                 int dstWidth, int dstHeight,
                 bool argb, VideoContentHint hint);

}

namespace LiveKitCpp
{

std::optional<webrtc::VideoFrame> createVideoFrame(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                                                   webrtc::VideoRotation rotation,
                                                   int64_t timeStampMicro, uint16_t id,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace)
{
    if (buff) {
        webrtc::VideoFrame::Builder builder;
        builder.set_video_frame_buffer(buff);
        builder.set_rotation(rotation);
        if (timeStampMicro > 0LL) {
            builder.set_timestamp_us(timeStampMicro);
        } else {
            builder.set_timestamp_us(rtc::TimeMicros());
        }
        auto frame = builder.build();
        frame.set_color_space(colorSpace);
        return frame;
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

bool scaleNV12(const uint8_t* srcY, int srcStrideY,
               const uint8_t* srcUV, int srcStrideUV,
               int srcWidth, int srcHeight,
               uint8_t* dstY, int dstStrideY,
               uint8_t* dstUV, int dstStrideUV,
               int dstWidth, int dstHeight,
               VideoContentHint hint)
{
    return cpuScaleNV12(srcY, srcStrideY,
                        srcUV, srcStrideUV,
                        srcWidth, srcHeight,
                        dstY, dstStrideY,
                        dstUV, dstStrideUV,
                        dstWidth, dstHeight,
                        hint);
}

bool scaleRGB(VideoFrameType type, const std::byte* srcRGB,
              int srcStrideRGB, int srcWidth, int srcHeight,
              std::byte* dstRGB, int dstStrideRGB,
              int dstWidth, int dstHeight,
              VideoContentHint hint)
{
    switch (type) {
        case VideoFrameType::RGB24:
        case VideoFrameType::BGR24:
            return scaleRGB24(srcRGB, srcStrideRGB, srcWidth, srcHeight,
                              dstRGB, dstStrideRGB, dstWidth, dstHeight,
                              hint);
        case VideoFrameType::BGRA32:
        case VideoFrameType::ARGB32:
        case VideoFrameType::RGBA32:
        case VideoFrameType::ABGR32:
            return scaleRGB32(srcRGB, srcStrideRGB, srcWidth, srcHeight,
                              dstRGB, dstStrideRGB, dstWidth, dstHeight,
                              hint);
        default:
            assert(false);
            break;
    }
    return false;
}


bool scaleRGB24(const std::byte* srcRGB, int srcStrideRGB,
                int srcWidth, int srcHeight,
                std::byte* dstRGB, int dstStrideRGB,
                int dstWidth, int dstHeight,
                VideoContentHint hint)
{
    if (gpuScaleRGB(srcRGB, srcStrideRGB, srcWidth, srcHeight,
                    dstRGB, dstStrideRGB, dstWidth, dstHeight,
                    false, hint)) {
        return true;
    }
    return cpuScaleRGB24(srcRGB, srcStrideRGB, srcWidth, srcHeight,
                         dstRGB, dstStrideRGB, dstWidth, dstHeight,
                         hint);
}

bool scaleRGB32(const std::byte* srcARGB, int srcStrideARGB,
                int srcWidth, int srcHeight,
                std::byte* dstARGB, int dstStrideARGB,
                int dstWidth, int dstHeight,
                VideoContentHint hint)
{
    if (gpuScaleRGB(srcARGB, srcStrideARGB, srcWidth, srcHeight,
                    dstARGB, dstStrideARGB, dstWidth, dstHeight,
                    true, hint)) {
        return true;
    }
    return cpuScaleRGB32(srcARGB, srcStrideARGB, srcWidth, srcHeight,
                         dstARGB, dstStrideARGB, dstWidth, dstHeight,
                         hint);
}

std::string toString(const VideoOptions& options)
{
    std::string desc;
    if (options) {
        desc = std::to_string(options._width) + "x" + std::to_string(options._height) +
               ": " + std::to_string(options._maxFPS) + "fps";
    }
    else {
        desc = "<null>";
    }
    if (options._type.has_value()) {
        desc += ", " + toString(options._type.value());
    }
    if (options._interlaced) {
        desc += ", interlaced ON";
    }
    return desc;
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

webrtc::VideoType map(VideoFrameType type)
{
    switch (type) {
        case VideoFrameType::I420:
            return webrtc::VideoType::kI420;
        case VideoFrameType::IYUV:
            return webrtc::VideoType::kIYUV;
        case VideoFrameType::RGB24:
            return webrtc::VideoType::kRGB24;
        case VideoFrameType::BGR24:
            return webrtc::VideoType::kBGR24;
        case VideoFrameType::ARGB32:
            return webrtc::VideoType::kARGB;
        case VideoFrameType::ABGR32:
            return webrtc::VideoType::kABGR;
        case VideoFrameType::RGB565:
            return webrtc::VideoType::kRGB565;
        case VideoFrameType::YUY2:
            return webrtc::VideoType::kYUY2;
        case VideoFrameType::YV12:
            return webrtc::VideoType::kYV12;
        case VideoFrameType::UYVY:
            return webrtc::VideoType::kUYVY;
        case VideoFrameType::MJPEG:
            return webrtc::VideoType::kMJPEG;
        case VideoFrameType::BGRA32:
            return webrtc::VideoType::kBGRA;
        case VideoFrameType::NV12:
            return webrtc::VideoType::kNV12;
        default:
            break;
    }
    return webrtc::VideoType::kUnknown;
}

std::optional<VideoFrameType> map(webrtc::VideoType type)
{
    switch (type) {
        case webrtc::VideoType::kUnknown:
            break;
        case webrtc::VideoType::kI420:
            return VideoFrameType::I420;
        case webrtc::VideoType::kIYUV:
            return VideoFrameType::IYUV;
        case webrtc::VideoType::kRGB24:
            return VideoFrameType::RGB24;
        case webrtc::VideoType::kBGR24:
            return VideoFrameType::BGR24;
        case webrtc::VideoType::kARGB:
            return VideoFrameType::ARGB32;
        case webrtc::VideoType::kABGR:
            return VideoFrameType::ABGR32;
        case webrtc::VideoType::kRGB565:
            return VideoFrameType::RGB565;
        case webrtc::VideoType::kYUY2:
            return VideoFrameType::YUY2;
        case webrtc::VideoType::kYV12:
            return VideoFrameType::YV12;
        case webrtc::VideoType::kUYVY:
            return VideoFrameType::UYVY;
        case webrtc::VideoType::kMJPEG:
            return VideoFrameType::MJPEG;
        case webrtc::VideoType::kBGRA:
            return VideoFrameType::BGRA32;
        case webrtc::VideoType::kNV12:
            return VideoFrameType::NV12;
        default:
            assert(false);
            break;
    }
    return {};
}

int fourcc(VideoFrameType type)
{
    return fourcc(map(type));
}

int fourcc(webrtc::VideoType type)
{
    return webrtc::ConvertVideoType(type);
}

} // namespace LiveKitCpp

namespace
{

bool cpuScaleNV12(const uint8_t* srcY, int srcStrideY,
                  const uint8_t* srcUV, int srcStrideUV,
                  int srcWidth, int srcHeight,
                  uint8_t* dstY, int dstStrideY,
                  uint8_t* dstUV, int dstStrideUV,
                  int dstWidth, int dstHeight,
                  VideoContentHint hint)
{
    return 0 == libyuv::NV12Scale(srcY, srcStrideY,
                                  srcUV, srcStrideUV,
                                  srcWidth, srcHeight,
                                  dstY, dstStrideY,
                                  dstUV, dstStrideUV,
                                  dstWidth, dstHeight,
                                  mapLibYUV(hint));
}

bool cpuScaleRGB24(const std::byte* srcRGB, int srcStrideRGB,
                   int srcWidth, int srcHeight,
                   std::byte* dstRGB, int dstStrideRGB,
                   int dstWidth, int dstHeight,
                   VideoContentHint hint)
{
    return 0 == libyuv::RGBScale(reinterpret_cast<const uint8_t*>(srcRGB),
                                 srcStrideRGB, srcWidth, srcHeight,
                                 reinterpret_cast<uint8_t*>(dstRGB),
                                 dstStrideRGB, dstWidth, dstHeight,
                                 mapLibYUV(hint));
}

bool cpuScaleRGB32(const std::byte* srcARGB, int srcStrideARGB,
                   int srcWidth, int srcHeight,
                   std::byte* dstARGB, int dstStrideARGB,
                   int dstWidth, int dstHeight,
                   VideoContentHint hint)
{
    return 0 == libyuv::ARGBScale(reinterpret_cast<const uint8_t*>(srcARGB),
                                  srcStrideARGB, srcWidth, srcHeight,
                                  reinterpret_cast<uint8_t*>(dstARGB),
                                  dstStrideARGB, dstWidth, dstHeight,
                                  mapLibYUV(hint));
}

bool gpuScaleRGB(const std::byte* /*src*/, int /*srcStride*/,
                 int /*srcWidth*/, int /*srcHeight*/,
                 std::byte* /*dst*/, int /*dstStride*/,
                 int /*dstWidth*/, int /*dstHeight*/,
                 bool /*argb*/, VideoContentHint /*hint*/)
{
/*#ifdef WEBRTC_MAC
    static thread_local MetalRGBScaler rgbScaler;
    if (rgbScaler.valid()) {
        if (rgbScaler.scale(src, srcStride, srcWidth, srcHeight, dst, dstStride, dstWidth, dstHeight)) {
            return true;
        }
    }
#endif*/
    return false;
}

}
