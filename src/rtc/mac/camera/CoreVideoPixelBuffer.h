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
};

} // namespace LiveKitCpp
#endif
