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
#pragma once // MFMediaSampleBuffer.h
#include "VideoFrameBufferPool.h"
#include <atlbase.h> //CComPtr support
#include <mfobjects.h>
#include <strmif.h>

namespace webrtc {
enum class VideoType;
struct VideoCaptureCapability;
} // namespace webrtc

namespace LiveKitCpp 
{

class MFMediaSampleBuffer
{
public:
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer> create(int width, int height, 
                                                               VideoFrameType bufferType,
                                                               BYTE* buffer, DWORD actualBufferLen, 
                                                               DWORD totalBufferLen,
                                                               const CComPtr<IMediaSample>& sample,
                                                               webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                                                               const VideoFrameBufferPool& framesPool = {});
    static const MFMediaSampleBuffer* mediaSampleBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer);
    virtual const BYTE* buffer() const = 0;
    virtual DWORD actualBufferLen() const = 0;
    virtual DWORD totalBufferLen() const { return actualBufferLen(); }
    virtual const CComPtr<IMediaSample> data() const = 0;
protected:
    virtual ~MFMediaSampleBuffer() = default;

};

} // namespace LiveKitCpp