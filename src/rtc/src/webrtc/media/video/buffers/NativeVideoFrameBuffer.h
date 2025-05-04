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
#pragma once // NativeVideoFrameBuffer.h
#include "VideoBufferHandleProvider.h"
#include "livekit/rtc/media/VideoFrameType.h"
#include <api/video/video_frame_buffer.h>

namespace LiveKitCpp
{

class NativeVideoFrameBuffer : public webrtc::VideoFrameBuffer,
                               virtual public VideoBufferHandleProvider
{
public:
    Type type() const final { return webrtc::VideoFrameBuffer::Type::kNative; }
    virtual VideoFrameType nativeType() const = 0;
    virtual int stride(size_t planeIndex) const = 0;
    virtual const std::byte* data(size_t planeIndex) const = 0;
    virtual int dataSize(size_t planeIndex) const { return stride(planeIndex) * height(); }
    virtual int dataSize() const;
};
	
} // namespace LiveKitCpp
