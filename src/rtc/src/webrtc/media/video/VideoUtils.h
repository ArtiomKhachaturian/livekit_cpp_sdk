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
#pragma once // VideoUtils.h
#include "CodecStatus.h"
#ifdef WEBRTC_MAC
#include "CFAutoRelease.h"
#include <CoreVideo/CVPixelBuffer.h>
#include <VideoToolbox/VideoToolbox.h>
#endif
#include "livekit/rtc/media/VideoContentHint.h"
#include <api/media_stream_interface.h>
#include <api/video/video_frame.h>
#include <api/video_codecs/sdp_video_format.h>
#include <modules/video_capture/video_capture_config.h> // for values in webrtc::videocapturemodule
#include <optional>

namespace webrtc {
enum class VideoType;
}

namespace LiveKitCpp
{

enum class VideoFrameType;

// new frames factory
// webrtc::VideoFrameBuffer::Type::kNative & webrtc::VideoFrameBuffer::Type::kI420A types
// are ignored - output frame will be with webrtc::VideoFrameBuffer::Type::kI420 buffer
std::optional<webrtc::VideoFrame> createVideoFrame(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                                                   webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                                                   int64_t timeStampMicro = 0LL,
                                                   uint16_t id = 0U,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace = {});

webrtc::VideoTrackInterface::ContentHint map(VideoContentHint hint);
VideoContentHint map(webrtc::VideoTrackInterface::ContentHint hint);

bool scaleNV12(const uint8_t* srcY, int srcStrideY,
               const uint8_t* srcUV, int srcStrideUV,
               int srcWidth, int srcHeight,
               uint8_t* dstY, int dstStrideY,
               uint8_t* dstUV, int dstStrideUV,
               int dstWidth, int dstHeight,
               VideoContentHint hint = VideoContentHint::None);
bool scaleRGB(VideoFrameType type, const std::byte* srcRGB,
              int srcStrideRGB, int srcWidth, int srcHeight,
              std::byte* dstRGB, int dstStrideRGB,
              int dstWidth, int dstHeight,
              VideoContentHint hint = VideoContentHint::None);
// 24 bpp
bool scaleRGB24(const std::byte* srcRGB, int srcStrideRGB,
                int srcWidth, int srcHeight,
                std::byte* dstRGB, int dstStrideRGB,
                int dstWidth, int dstHeight,
                VideoContentHint hint = VideoContentHint::None);
// 32bpp
bool scaleRGB32(const std::byte* srcARGB, int srcStrideARGB,
                int srcWidth, int srcHeight,
                std::byte* dstARGB, int dstStrideARGB,
                int dstWidth, int dstHeight,
                VideoContentHint hint = VideoContentHint::None);

size_t planesCount(VideoFrameType type);

// VPX
bool isVP8VideoFormat(const webrtc::SdpVideoFormat& format);
bool isVP8CodecName(const std::string& codecName);
bool isVP9VideoFormat(const webrtc::SdpVideoFormat& format);
bool isVP9CodecName(const std::string& codecName);
CodecStatus encoderStatus(const webrtc::SdpVideoFormat& format);
CodecStatus decoderStatus(const webrtc::SdpVideoFormat& format);
std::vector<webrtc::SdpVideoFormat> mergeFormats(std::vector<webrtc::SdpVideoFormat> f1,
                                                 std::vector<webrtc::SdpVideoFormat> f2);
template <class TCodecFactory>
inline std::vector<webrtc::SdpVideoFormat> mergeFormats(const TCodecFactory* factory,
                                                        std::vector<webrtc::SdpVideoFormat> f2)
{
    if (factory) {
        return mergeFormats(factory->GetSupportedFormats(), std::move(f2));
    }
    return f2;
}
#ifdef WEBRTC_MAC
// constants
constexpr CMVideoCodecType codecTypeVP9() { return kCMVideoCodecType_VP9; }
constexpr CMVideoCodecType codecTypeH264() { return kCMVideoCodecType_H264; }
// https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding
constexpr CMVideoCodecType codecTypeH265() { return kCMVideoCodecType_HEVC; }
// NV12
// full NV12 format is suitable as default pixel format for VT codecs
constexpr OSType formatNV12Full() { return kCVPixelFormatType_420YpCbCr8BiPlanarFullRange; }   // luma=[0,255] chroma=[1,255]
constexpr OSType formatNV12Video() { return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange; } // luma=[16,235] chroma=[16,240]
// YUY2 packed Y'CbY'Cr
constexpr OSType formatYUY2() { return kCVPixelFormatType_422YpCbCr8_yuvs; }
// I420
constexpr OSType formatI420() { return kCVPixelFormatType_420YpCbCr8Planar; }
// UYVY
constexpr OSType formatUYVY() { return kCVPixelFormatType_422YpCbCr8; }
// RGB 24bpp
constexpr OSType formatRGB24() { return kCVPixelFormatType_24RGB; }
constexpr OSType formatBGR24() { return kCVPixelFormatType_24BGR; }
// RGB 32bpp
constexpr OSType formatBGRA32() { return kCVPixelFormatType_32BGRA; }
constexpr OSType formatARGB32() { return kCVPixelFormatType_32ARGB; }
constexpr OSType formatRGBA32() { return kCVPixelFormatType_32RGBA; }
constexpr OSType formatABGR32() { return kCVPixelFormatType_32ABGR; }
bool isNV12Format(OSType format);
bool isRGB24Format(OSType format);
bool isRGB32Format(OSType format);
inline bool isRGBFormat(OSType format) { return isRGB24Format(format) || isRGB32Format(format); }
bool isSupportedFormat(OSType format);
// codec type conversion
std::optional<CMVideoCodecType> toVTCodecType(webrtc::VideoCodecType codecType);
std::optional<webrtc::VideoCodecType> fromVTCodecType(CMVideoCodecType codecType);
std::string videoCodecTypeToString(FourCharCode fccCodecTtype); // alias of CMVideoCodecType
CFDictionaryRefAutoRelease createImageBufferAttributes(const void* pixelFormats, int32_t width, int32_t height);
#endif

webrtc::VideoType map(VideoFrameType type);
std::optional<VideoFrameType> map(webrtc::VideoType type);
int fourcc(VideoFrameType type);
int fourcc(webrtc::VideoType type);
	
} // namespace LiveKitCpp
