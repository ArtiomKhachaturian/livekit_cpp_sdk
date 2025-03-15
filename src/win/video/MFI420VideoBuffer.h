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
#include "MFVideoBuffer.h"
#include <api/video/i420_buffer.h>
#include <memory>

namespace libyuv {
    extern "C" {
        extern int I420ToNV12(const uint8_t* src_y,
            int src_stride_y,
            const uint8_t* src_u,
            int src_stride_u,
            const uint8_t* src_v,
            int src_stride_v,
            uint8_t* dst_y,
            int dst_stride_y,
            uint8_t* dst_uv,
            int dst_stride_uv,
            int width,
            int height);
    }
}

namespace LiveKitCpp 
{

template <class TMFData>
class MFI420VideoBuffer : public MFVideoBuffer<webrtc::I420BufferInterface, TMFData>
{
    using BaseClass = MFVideoBuffer<webrtc::I420BufferInterface, TMFData>;
public:
    MFI420VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      const CComPtr<TMFData>& data);
    // impl. of MFVideoBufferInterface
    rtc::scoped_refptr<webrtc::NV12BufferInterface> toNV12() final;
    // impl. of webrtc::I420BufferInterface
    int StrideY() const final { return BaseClass::width(); }
    int StrideU() const final { return (BaseClass::width() + 1) / 2; }
    int StrideV() const final { return StrideU(); }
    const uint8_t* DataY() const final { return BaseClass::buffer(); }
    const uint8_t* DataU() const final;
    const uint8_t* DataV() const final;
};

template <class TMFData>
inline MFI420VideoBuffer<TMFData>::MFI420VideoBuffer(int width, int height, BYTE* buffer,
                                                     DWORD actualBufferLen, DWORD totalBufferLen,
                                                     const CComPtr<TMFData>& data)
    : BaseClass(width, height, webrtc::VideoType::kI420, buffer, actualBufferLen, totalBufferLen, data)
{
}

template <class TMFData>
inline rtc::scoped_refptr<webrtc::NV12BufferInterface> MFI420VideoBuffer<TMFData>::toNV12()
{
    const auto nv12 = createNV12(BaseClass::width(), BaseClass::height());
    if (nv12 && 0 == libyuv::I420ToNV12(DataY(), StrideY(), 
                                        DataU(), StrideU(),
                                        DataV(), StrideV(),
                                        nv12->MutableDataY(), nv12->StrideY(),
                                        nv12->MutableDataUV(), nv12->StrideUV(),
                                        nv12->width(), nv12->height())) {
        return nv12;
    }
    return {};
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
#endif