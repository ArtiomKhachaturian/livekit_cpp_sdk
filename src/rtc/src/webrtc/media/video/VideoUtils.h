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
#include "livekit/rtc/media/VideoContentHint.h"
#include <api/media_stream_interface.h>
#include <api/video/video_frame.h>
#ifdef WEBRTC_MAC
#include <CoreVideo/CVPixelBuffer.h>
#endif
#include <optional>

namespace LiveKitCpp
{

// new frames factory
// webrtc::VideoFrameBuffer::Type::kNative & webrtc::VideoFrameBuffer::Type::kI420A types
// are ignored - output frame will be with webrtc::VideoFrameBuffer::Type::kI420 buffer
std::optional<webrtc::VideoFrame> createVideoFrame(int width, int height,
                                                   webrtc::VideoFrameBuffer::Type type = webrtc::VideoFrameBuffer::Type::kI420,
                                                   int64_t timeStampMicro = 0LL,
                                                   uint16_t id = 0U,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace = {});
std::optional<webrtc::VideoFrame> createVideoFrame(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                                                   int64_t timeStampMicro = 0LL,
                                                   uint16_t id = 0U,
                                                   const std::optional<webrtc::ColorSpace>& colorSpace = {});
std::optional<webrtc::VideoFrame> createBlackVideoFrame(int width, int height,
                                                        int64_t timeStampMicro = 0LL,
                                                        uint16_t id = 0U,
                                                        const std::optional<webrtc::ColorSpace>& colorSpace = {});
webrtc::VideoTrackInterface::ContentHint map(VideoContentHint hint);
VideoContentHint map(webrtc::VideoTrackInterface::ContentHint hint);

#ifdef WEBRTC_MAC
// constants
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
// RGB24
constexpr OSType formatRGB24() { return kCVPixelFormatType_24RGB; }
constexpr OSType formatBGR24() { return kCVPixelFormatType_24BGR; }
// RGB32
constexpr OSType formatBGRA32() { return kCVPixelFormatType_32BGRA; }
constexpr OSType formatARGB32() { return kCVPixelFormatType_32ARGB; }
constexpr OSType formatRGBA32() { return kCVPixelFormatType_32RGBA; }
bool isNV12Format(OSType format);
bool isRGB24Format(OSType format);
bool isRGB32Format(OSType format);
inline bool isRGBFormat(OSType format) { return isRGB24Format(format) || isRGB32Format(format); }
bool isSupportedFormat(OSType format);
#endif
	
} // namespace LiveKitCpp
