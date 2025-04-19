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
#include <api/video/i420_buffer.h>
#include <libyuv/convert.h>
#include <modules/desktop_capture/desktop_frame.h>

namespace LiveKitCpp
{

DesktopFrameVideoBuffer::DesktopFrameVideoBuffer(std::unique_ptr<webrtc::DesktopFrame> frame)
    : _frame(std::move(frame))
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

int DesktopFrameVideoBuffer::dataSize(size_t planeIndex) const
{
    if (0U == planeIndex && _frame) {
        return _frame->stride() * _frame->size().height();
    }
    return 0;
}

rtc::scoped_refptr<webrtc::I420BufferInterface> DesktopFrameVideoBuffer::convertToI420() const
{
    if (_frame) {
        const auto& size = _frame->size();
        if (auto target = webrtc::I420Buffer::Create(size.width(), size.height())) {
            if (0 == libyuv::ARGBToI420(_frame->data(), _frame->stride(),
                                        target->MutableDataY(), target->StrideY(),
                                        target->MutableDataU(), target->StrideU(),
                                        target->MutableDataV(), target->StrideV(),
                                        size.width(), size.height())) {
                return target;
            }
        }
    }
    return {};
}

int DesktopFrameVideoBuffer::width() const
{
    return _frame ? _frame->size().width() : 0;
}

int DesktopFrameVideoBuffer::height() const
{
    return _frame ? _frame->size().height() : 0;
}

} // namespace LiveKitCpp
