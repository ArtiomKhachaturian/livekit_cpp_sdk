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
#include "DesktopFrameVideoBuffer.h"
#include <modules/desktop_capture/desktop_frame.h>

namespace LiveKitCpp
{

DesktopFrameVideoBuffer::DesktopFrameVideoBuffer(std::unique_ptr<webrtc::DesktopFrame> frame,
                                                 VideoFrameBufferPool framesPool)
    : RgbGenericVideoFrameBuffer(rgbType(), std::move(framesPool))
    , _frame(std::move(frame))
{
}

DesktopFrameVideoBuffer::DesktopFrameVideoBuffer(int width, int height,
                                                 VideoFrameBufferPool framesPool)
    : DesktopFrameVideoBuffer(make(width, height), std::move(framesPool))
{
}

DesktopFrameVideoBuffer::~DesktopFrameVideoBuffer()
{
}

int DesktopFrameVideoBuffer::stride(size_t planeIndex) const
{
    if (0U == planeIndex && _frame) {
        return _frame->stride();
    }
    return 0;
}

const std::byte* DesktopFrameVideoBuffer::data(size_t planeIndex) const
{
    if (0U == planeIndex && _frame) {
        return reinterpret_cast<const std::byte*>(_frame->data());
    }
    return nullptr;
}

int DesktopFrameVideoBuffer::width() const
{
    return _frame ? _frame->size().width() : 0;
}

int DesktopFrameVideoBuffer::height() const
{
    return _frame ? _frame->size().height() : 0;
}

std::unique_ptr<webrtc::DesktopFrame> DesktopFrameVideoBuffer::make(int width, int height)
{
    if (width > 0 && height > 0) {
        return std::make_unique<webrtc::BasicDesktopFrame>(webrtc::DesktopSize{width, height});
    }
    return {};
}

std::string DesktopFrameVideoBuffer::storage_representation() const
{
    return "LiveKitCpp::DesktopFrameVideoBuffer";
}

} // namespace LiveKitCpp
