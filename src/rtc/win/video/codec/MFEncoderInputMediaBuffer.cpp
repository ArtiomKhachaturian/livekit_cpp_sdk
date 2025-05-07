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
#include "MFEncoderInputMediaBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "LibyuvImport.h"
#include <absl/container/inlined_vector.h>

namespace LiveKitCpp 
{

MFEncoderInputMediaBuffer::MFEncoderInputMediaBuffer(rtc::scoped_refptr<webrtc::NV12BufferInterface> buffer)
    : _buffer(std::move(buffer))
    , _size(webrtc::CalcBufferSize(webrtc::VideoType::kNV12, _buffer->width(), _buffer->height()))
{
}

MFEncoderInputMediaBuffer::~MFEncoderInputMediaBuffer()
{
}

IMFMediaBuffer* MFEncoderInputMediaBuffer::create(const ::rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer,
                                                  const VideoFrameBufferPool& pool)
{
    if (auto nv12 = NV12VideoFrameBuffer::toNV12(buffer, pool)) {
        return new MFEncoderInputMediaBuffer(std::move(nv12));
    }
    return nullptr;
}

IMFMediaBuffer* MFEncoderInputMediaBuffer::create(const webrtc::VideoFrame& frame, const VideoFrameBufferPool& pool)
{
    return create(frame.video_frame_buffer(), pool);
}

HRESULT MFEncoderInputMediaBuffer::GetCurrentLength(DWORD* pcbCurrentLength)
{
    if (pcbCurrentLength) {
        *pcbCurrentLength = _size;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT MFEncoderInputMediaBuffer::GetMaxLength(DWORD* pcbMaxLength)
{
    if (pcbMaxLength) {
        *pcbMaxLength = _size;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT MFEncoderInputMediaBuffer::Lock(BYTE** ppbBuffer, DWORD* pcbMaxLength, DWORD* pcbCurrentLength)
{
    if (ppbBuffer || pcbMaxLength || pcbCurrentLength) {
        if (ppbBuffer) {
            const auto addr = reinterpret_cast<const BYTE*>(_buffer->DataY());
            *ppbBuffer = const_cast<BYTE*>(addr);
        }
        if (pcbMaxLength) {
            *pcbMaxLength = _size;
        }
        if (pcbCurrentLength) {
            *pcbCurrentLength = _size;
        }
        return S_OK;
    }
    return E_INVALIDARG;
}

} // namespace LiveKitCpp