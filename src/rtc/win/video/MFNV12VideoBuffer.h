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
#pragma once // MFNV12VideoBuffer.h
#include "MFVideoBuffer.h"
#include "NV12VideoFrameBuffer.h"

namespace LiveKitCpp 
{

template <class TMFData>
class MFNV12VideoBuffer : public MFVideoBuffer<NV12VideoFrameBuffer, TMFData>
{
    using BaseClass = MFVideoBuffer<NV12VideoFrameBuffer, TMFData>;
public:
    MFNV12VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      const CComPtr<IMFMediaBuffer>& data);
    // impl. of NV12VideoFrameBuffer
    const uint8_t* DataY() const final;
    const uint8_t* DataUV() const final;
};

template <class TMFData>
inline MFNV12VideoBuffer<TMFData>::MFNV12VideoBuffer(int width, int height, BYTE* buffer,
                                                     DWORD actualBufferLen, DWORD totalBufferLen,
                                                     const CComPtr<IMFMediaBuffer>& data)
    : BaseClass(width, height, webrtc::VideoType::kNV12, buffer, actualBufferLen, totalBufferLen, data)
{
}

template <class TMFData>
inline const uint8_t* MFNV12VideoBuffer<TMFData>::DataY() const
{ 
    return NV12VideoFrameBuffer::nv12DataY(BaseClass::buffer());
}

template <class TMFData>
inline const uint8_t* MFNV12VideoBuffer<TMFData>::DataUV() const
{ 
    return NV12VideoFrameBuffer::nv12DataUV(BaseClass::buffer(), 
                                            BaseClass::width(), 
                                            BaseClass::height());
}

} // namespace LiveKitCpp