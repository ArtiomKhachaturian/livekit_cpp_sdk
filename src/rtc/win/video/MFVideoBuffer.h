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
#include "MFMediaBufferLocker.h"
#include "livekit/rtc/media/VideoFrameType.h"
#include <atlbase.h> //CComPtr support
#include <mfobjects.h>
#include <strmif.h>

namespace LiveKitCpp 
{

class MFVideoBuffer
{
public:
    virtual const BYTE* buffer() const = 0;
    virtual DWORD actualBufferLen() const = 0;
protected:
    virtual ~MFVideoBuffer() = default;
};

class MFMediaSampleBuffer : public MFVideoBuffer
{
public:
    // I420 + NV12 + all non-planar types are supported
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer> 
        create(int width, int height, VideoFrameType bufferType,
               BYTE* buffer, DWORD actualBufferLen, 
               const CComPtr<IMediaSample>& sample,
               webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
               const VideoFrameBufferPool& framesPool = {});
    static const MFMediaSampleBuffer* sampleBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer);
protected:
    virtual ~MFMediaSampleBuffer() = default;
};

class MFMediaBuffer : public MFVideoBuffer
{
public:
    // I420 + NV12 + all non-planar types are supported
    static rtc::scoped_refptr<webrtc::VideoFrameBuffer>
        create(int width, int height, 
               VideoFrameType bufferType,
               MFMediaBufferLocker mediaBufferLocker,
               webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
               const VideoFrameBufferPool& framesPool = {});
    static const MFMediaBuffer* mediaBuffer(const rtc::scoped_refptr<webrtc::NV12BufferInterface>& buffer);
protected:
    virtual ~MFMediaBuffer() = default;
};

} // namespace LiveKitCpp