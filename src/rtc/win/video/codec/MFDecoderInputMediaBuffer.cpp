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
#include "MFDecoderInputMediaBuffer.h"

namespace LiveKitCpp 
{

MFDecoderInputMediaBuffer::MFDecoderInputMediaBuffer(const rtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& buffer)
    : _buffer(buffer)
{
}

IMFMediaBuffer* MFDecoderInputMediaBuffer::create(const rtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& buffer)
{
    if (buffer) {
        return new MFDecoderInputMediaBuffer(buffer);
    }
    return NULL;
}

HRESULT MFDecoderInputMediaBuffer::GetCurrentLength(DWORD* pcbCurrentLength)
{
    if (pcbCurrentLength) {
        *pcbCurrentLength = _buffer->size();
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT MFDecoderInputMediaBuffer::GetMaxLength(DWORD* pcbMaxLength)
{
    if (pcbMaxLength) {
        *pcbMaxLength = _buffer->size();
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT MFDecoderInputMediaBuffer::Lock(BYTE** ppbBuffer, DWORD* pcbMaxLength, DWORD* pcbCurrentLength)
{
    if (ppbBuffer || pcbMaxLength || pcbCurrentLength) {
        if (ppbBuffer) {
            *ppbBuffer = _buffer->data();
        }
        if (pcbMaxLength) {
            *pcbMaxLength = _buffer->size();
        }
        if (pcbCurrentLength) {
            *pcbCurrentLength = _buffer->size();
        }
        return S_OK;
    }
    return E_INVALIDARG;
}

} // namespace LiveKitCpp