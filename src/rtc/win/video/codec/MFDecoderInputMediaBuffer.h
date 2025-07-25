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
#pragma once // MFDecoderInputMediaBuffer.h
#ifdef USE_PLATFORM_DECODERS
#include "MFMediaBuffer.h"
#include <api/video/encoded_image.h>

namespace LiveKitCpp 
{

class MFDecoderInputMediaBuffer : public MFMediaBuffer
{
public:
    static IMFMediaBuffer* create(const rtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& buffer);
    // impl. of IMFMediaBuffer
    HRESULT STDMETHODCALLTYPE GetCurrentLength(DWORD* pcbCurrentLength) final;
    HRESULT STDMETHODCALLTYPE GetMaxLength(DWORD* pcbMaxLength) final;
    HRESULT STDMETHODCALLTYPE Lock(BYTE** ppbBuffer, DWORD* pcbMaxLength, DWORD* pcbCurrentLength) final;
private:
    MFDecoderInputMediaBuffer(const rtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& buffer);
protected:
    // impl. of IUnknownImpl
    void releaseThis() final { delete this; }
private:
    const rtc::scoped_refptr<webrtc::EncodedImageBufferInterface> _buffer;
};

} // namespace LiveKitCpp
#endif