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
#include <CoreVideo/CoreVideo.h>

namespace LiveKitCpp
{

// constants
// NV12
// full NV12 format is suitable as default pixel format for VT codecs
inline constexpr OSType pixelFormatNV12Full() { return kCVPixelFormatType_420YpCbCr8BiPlanarFullRange; }   // luma=[0,255] chroma=[1,255]
inline constexpr OSType pixelFormatNV12Video() { return kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange; } // luma=[16,235] chroma=[16,240]
bool isNV12Format(OSType format);
// YUY2 packed Y'CbY'Cr
inline constexpr OSType pixelFormatYUY2() { return kCVPixelFormatType_422YpCbCr8_yuvs; }
// I420
inline constexpr OSType pixelFormatI420() { return kCVPixelFormatType_420YpCbCr8Planar; }
// UYVY
inline constexpr OSType pixelFormatUYVY() { return kCVPixelFormatType_422YpCbCr8; }
// RGB24
inline constexpr OSType pixelFormatRGB24() { return kCVPixelFormatType_24RGB; }
inline constexpr OSType pixelFormatBGR24() { return kCVPixelFormatType_24BGR; }
bool isRGB24Format(OSType format);
// RGB32
inline constexpr OSType pixelFormatBGRA32() { return kCVPixelFormatType_32BGRA; }
inline constexpr OSType pixelFormatARGB32() { return kCVPixelFormatType_32ARGB; }
inline constexpr OSType pixelFormatRGBA32() { return kCVPixelFormatType_32RGBA; }
bool isRGB32Format(OSType format);
// common testers
inline constexpr bool isRGBFormat(OSType format) { return isRGB24Format(format) || isRGB32Format(format); }
inline constexpr bool isSupportedFormat(OSType format) { return isNV12Format(format) || isRGBFormat(format); }


} // namespace LiveKitCpp

#endif
