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
#pragma once // DesktopFrameVideoBuffer.h
#include "RgbGenericVideoFrameBuffer.h"
#include <memory>

namespace webrtc {
class DesktopFrame;
}

namespace LiveKitCpp
{

class DesktopFrameVideoBuffer : public RgbGenericVideoFrameBuffer
{
public:
    DesktopFrameVideoBuffer(std::unique_ptr<webrtc::DesktopFrame> frame,
                            VideoFrameBufferPool framesPool = {});
    DesktopFrameVideoBuffer(int width, int height,
                            VideoFrameBufferPool framesPool = {});
    ~DesktopFrameVideoBuffer() override;
    // DesktopFrame objects always hold BGRA data (according to WebRTC docs)
    static constexpr VideoFrameType rgbType() { return VideoFrameType::BGRA32; }
    // impl. of NativeVideoFrameBuffer
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    // impl. of webrtc::VideoFrameBuffer
    int width() const final;
    int height() const final;
private:
    static std::unique_ptr<webrtc::DesktopFrame> make(int width, int height);
private:
    const std::unique_ptr<webrtc::DesktopFrame> _frame;
};
	
} // namespace LiveKitCpp
