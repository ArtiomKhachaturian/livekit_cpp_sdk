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
#pragma once // MFEncoderInputMediaBuffer.h
#ifdef USE_PLATFORM_ENCODERS
#include "VideoFrameBufferPool.h"
#include "MFMediaBuffer.h"
#include <api/video/video_frame.h>
#include <api/video/video_frame_buffer.h>
#include <atlbase.h> //CComPtr support

namespace LiveKitCpp 
{

// NV12 buffer
class MFEncoderInputMediaBuffer : public MFMediaBuffer
{
public:
    static CComPtr<IMFMediaBuffer> create(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer,
                                          const VideoFrameBufferPool& pool = {});
    static CComPtr<IMFMediaBuffer> create(const webrtc::VideoFrame& frame,
                                          const VideoFrameBufferPool& pool = {});
    ~MFEncoderInputMediaBuffer();
    // impl. of IMFMediaBuffer
    HRESULT STDMETHODCALLTYPE GetCurrentLength(DWORD* pcbCurrentLength) final;
    HRESULT STDMETHODCALLTYPE GetMaxLength(DWORD* pcbMaxLength) final;
    HRESULT STDMETHODCALLTYPE Lock(BYTE** ppbBuffer, DWORD* pcbMaxLength, DWORD* pcbCurrentLength) final;
protected:
    // impl. of IUnknownImpl
    void releaseThis() final { delete this; }
private:
    MFEncoderInputMediaBuffer(rtc::scoped_refptr<webrtc::NV12BufferInterface> buffer);
private:
    const rtc::scoped_refptr<webrtc::NV12BufferInterface> _buffer;
    const DWORD _size;
};

} // namespace LiveKitCpp
#endif