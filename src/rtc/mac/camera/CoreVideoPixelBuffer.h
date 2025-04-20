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
#pragma once
#ifdef WEBRTC_MAC
#include <CoreMedia/CMSampleBuffer.h>
#include <CoreVideo/CoreVideo.h>
#include <api/video/i420_buffer.h>
#include <memory>
#include <optional>

namespace LiveKitCpp
{

class CoreVideoPixelBuffer
{
public:
    static bool supported(CVPixelBufferRef buffer);
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer>
        create(CVPixelBufferRef buffer, bool retain = true);
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer>
        createFromSampleBuffer(CMSampleBufferRef buffer);
    static CVPixelBufferRef pixelBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& videoPixelBuffer,
                                        bool retain = true);
    // constants
    // NV12
    // full NV12 format is suitable as default pixel format for VT codecs
    static constexpr OSType formatNV12Full() { return kCVPixelFormatType_420YpCbCr8BiPlanarFullRange; }   // luma=[0,255] chroma=[1,255]
    static constexpr OSType formatNV12Video() { return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange; } // luma=[16,235] chroma=[16,240]
    // YUY2 packed Y'CbY'Cr
    static constexpr OSType formatYUY2() { return kCVPixelFormatType_422YpCbCr8_yuvs; }
    // I420
    static constexpr OSType formatI420() { return kCVPixelFormatType_420YpCbCr8Planar; }
    // UYVY
    static constexpr OSType formatUYVY() { return kCVPixelFormatType_422YpCbCr8; }
    // RGB24
    static constexpr OSType formatRGB24() { return kCVPixelFormatType_24RGB; }
    static constexpr OSType formatBGR24() { return kCVPixelFormatType_24BGR; }
    // RGB32
    static constexpr OSType formatBGRA32() { return kCVPixelFormatType_32BGRA; }
    static constexpr OSType formatARGB32() { return kCVPixelFormatType_32ARGB; }
    static constexpr OSType formatRGBA32() { return kCVPixelFormatType_32RGBA; }
    static bool isNV12Format(OSType format);
    static bool isRGB24Format(OSType format);
    static bool isRGB32Format(OSType format);
    static bool isRGBFormat(OSType format) { return isRGB24Format(format) || isRGB32Format(format); }
    static bool isSupportedFormat(OSType format);

};

} // namespace LiveKitCpp
#endif
