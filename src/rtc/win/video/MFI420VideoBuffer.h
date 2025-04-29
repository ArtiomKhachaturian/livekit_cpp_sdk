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
#include "MFVideoBuffer.h"
#include "LibyuvImport.h"
#include "VideoFrameBufferPool.h"
#include <api/video/i420_buffer.h>
#include <memory>

namespace LiveKitCpp 
{

template <class TMFData>
class MFI420VideoBuffer : public MFVideoBuffer<webrtc::I420BufferInterface, TMFData>
{
    using BaseClass = MFVideoBuffer<webrtc::I420BufferInterface, TMFData>;
public:
    MFI420VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      const CComPtr<TMFData>& data,
                      VideoFrameBufferPool framesPool = {});
    // impl. of webrtc::I420BufferInterface
    int StrideY() const final { return BaseClass::width(); }
    int StrideU() const final { return (BaseClass::width() + 1) / 2; }
    int StrideV() const final { return StrideU(); }
    const uint8_t* DataY() const final { return BaseClass::buffer(); }
    const uint8_t* DataU() const final;
    const uint8_t* DataV() const final;
private:
    const VideoFrameBufferPool _framesPool;
};

template <class TMFData>
inline MFI420VideoBuffer<TMFData>::MFI420VideoBuffer(int width, int height, BYTE* buffer,
                                                     DWORD actualBufferLen, DWORD totalBufferLen,
                                                     const CComPtr<TMFData>& data,
                                                     VideoFrameBufferPool framesPool)
    : BaseClass(width, height, buffer, actualBufferLen, totalBufferLen, data)
    , _framesPool(std::move(framesPool))
{
}

template <class TMFData>
inline const uint8_t* MFI420VideoBuffer<TMFData>::DataU() const 
{ 
    return BaseClass::buffer() + StrideY() * BaseClass::height();
}

template <class TMFData>
inline const uint8_t* MFI420VideoBuffer<TMFData>::DataV() const 
{ 
    const auto h = BaseClass::height();
    return BaseClass::buffer() + StrideY() * h + StrideU() * ((h + 1) / 2);
}

} // namespace LiveKitCpp