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
#ifdef WEBRTC_WIN
#include <Windows.h>
#include <api/video/video_frame_buffer.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>

namespace LiveKitCpp 
{

class MFVideoBufferInterface
{
public:
    // return NULL if conversion failed or impossible
    virtual rtc::scoped_refptr<webrtc::NV12BufferInterface> toNV12() = 0;
    virtual webrtc::VideoType bufferType() const = 0;
    virtual const BYTE* buffer() const = 0;
    virtual DWORD actualBufferLen() const = 0;
    virtual DWORD totalBufferLen() const { return actualBufferLen(); }
protected:
    virtual ~MFVideoBufferInterface() = default;
};

} // namespace LiveKitCpp
#endif