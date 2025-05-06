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
#include "MFMediaSampleBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "NativeVideoFrameBuffer.h"
#include "LibyuvImport.h"
#include "VideoUtils.h"
#include <api/make_ref_counted.h>
#include <modules/video_capture/video_capture_defines.h>
#include <cassert>

namespace 
{

using namespace LiveKitCpp;

libyuv::RotationMode fromVideoFrameRotation(webrtc::VideoRotation rotation);
bool contains(webrtc::VideoFrameBuffer::Type type, const rtc::ArrayView<webrtc::VideoFrameBuffer::Type>& types);

template <class TBaseBuffer>
class MFVideoBuffer : public TBaseBuffer, public MFMediaSampleBuffer
{
public:
    // impl. of VideoFrameBuffer
    int width() const final { return _width; }
    int height() const final { return _height; }
    // impl. of MFMediaSampleBuffer
    const BYTE* buffer() const final { return _buffer; }
    DWORD actualBufferLen() const final { return _actualBufferLen; }
    DWORD totalBufferLen() const final { return _totalBufferLen; }
    const CComPtr<IMediaSample> data() const final { return _data;  }
protected:
    template <class... Types>
    MFVideoBuffer(int width, int height, BYTE* buffer,
                  DWORD actualBufferLen, DWORD totalBufferLen,
                  const CComPtr<IMediaSample>& data,
                  Types&&... args);
private:
    const int _width;
    const int _height;
    const CComPtr<IMediaSample> _data;
    BYTE* const _buffer;
    const DWORD _actualBufferLen;
    const DWORD _totalBufferLen;
};

class MFI420VideoBuffer : public MFVideoBuffer<webrtc::I420BufferInterface>
{
    using Base = MFVideoBuffer<webrtc::I420BufferInterface>;
public:
    MFI420VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      const CComPtr<IMediaSample>& data,
                      VideoFrameBufferPool framesPool);
    // impl. of webrtc::I420BufferInterface
    int StrideY() const final { return Base::width(); }
    int StrideU() const final { return (Base::width() + 1) / 2; }
    int StrideV() const final { return StrideU(); }
    const uint8_t* DataY() const final { return Base::buffer(); }
    const uint8_t* DataU() const final;
    const uint8_t* DataV() const final;
private:
    const VideoFrameBufferPool _framesPool;
};

class MFNV12VideoBuffer : public MFVideoBuffer<NV12VideoFrameBuffer>
{
    using Base = MFVideoBuffer<NV12VideoFrameBuffer>;
public:
    MFNV12VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      const CComPtr<IMediaSample>& data,
                      VideoFrameBufferPool framesPool);
    // impl. of NV12VideoFrameBuffer
    const uint8_t* DataY() const final;
    const uint8_t* DataUV() const final;
};

class MFNativeBuffer : public MFVideoBuffer<VideoFrameBuffer<NativeVideoFrameBuffer>>
{
    using Base = MFVideoBuffer<VideoFrameBuffer<NativeVideoFrameBuffer>>;
public:
    MFNativeBuffer(int width, int height, BYTE* buffer,
                   DWORD actualBufferLen, DWORD totalBufferLen,
                   const CComPtr<IMediaSample>& data,
                   VideoFrameType bufferType,
                   webrtc::VideoRotation rotation,
                   VideoFrameBufferPool framesPool);
    // impl. of NativeVideoFrameBuffer
    VideoFrameType nativeType() const final { return _bufferType; }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
private:
    static int targetWidth(int width, int height, webrtc::VideoRotation rotation);
    static int targetHeight(int width, int height, webrtc::VideoRotation rotation);
    static void swapIfRotated(webrtc::VideoRotation rotation, int& width, int& height);
    // impl. of VideoFrameBuffer<>
    rtc::scoped_refptr<webrtc::I420BufferInterface> convertToI420() const final;
private:
    const int _originalWidth;
    const int _originalHeight;
    const webrtc::VideoRotation _rotation;
    const VideoFrameType _bufferType;
};


} // namespace

namespace LiveKitCpp 
{

rtc::scoped_refptr<webrtc::VideoFrameBuffer> MFMediaSampleBuffer::
    create(int width, int height, VideoFrameType bufferType, BYTE* buffer, DWORD actualBufferLen,
           DWORD totalBufferLen, const CComPtr<IMediaSample>& sample, webrtc::VideoRotation rotation,
           const VideoFrameBufferPool& framesPool)
{
    if (width > 0 && buffer && actualBufferLen && sample) {
        // setting absolute height (in case it was negative),
        // in Windows, the image starts bottom left, instead of top left,
        // setting a negative source height, inverts the image (within LibYuv),
        // see also [translateMediaTypeToVideoCaptureCapability] function for RGB24 cases
        const int absHeight = std::abs(height);
        if (absHeight > 0) {
            if (VideoFrameType::MJPEG != bufferType) {
                assert(actualBufferLen == webrtc::CalcBufferSize(map(bufferType), width, absHeight));
            }
            switch (bufferType) {
                case VideoFrameType::I420:
                    return webrtc::make_ref_counted<MFI420VideoBuffer>(width, absHeight,
                                                                      buffer, actualBufferLen, 
                                                                      totalBufferLen, sample, framesPool);
                case VideoFrameType::NV12:
                    return webrtc::make_ref_counted<MFNV12VideoBuffer>(width, absHeight,
                                                                      buffer, actualBufferLen,
                                                                      totalBufferLen, sample, framesPool);
                default:
                    break;
            }
            return webrtc::make_ref_counted<MFNativeBuffer>(width, absHeight,
                                                            buffer, actualBufferLen,
                                                            totalBufferLen, sample, bufferType, 
                                                            rotation, framesPool);
        }
    }
    return {};
}

const MFMediaSampleBuffer* MFMediaSampleBuffer::mediaSampleBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer)
{
    return dynamic_cast<const MFMediaSampleBuffer*>(buffer.get());
}

} // namespace LiveKitCpp

namespace
{

libyuv::RotationMode fromVideoFrameRotation(webrtc::VideoRotation rotation)
{
    switch (rotation) {
        case webrtc::VideoRotation::kVideoRotation_0:
            break;
        case webrtc::VideoRotation::kVideoRotation_90:
            return libyuv::RotationMode::kRotate90;
        case webrtc::VideoRotation::kVideoRotation_180:
            return libyuv::RotationMode::kRotate180;
        case webrtc::VideoRotation::kVideoRotation_270:
            return libyuv::RotationMode::kRotate270;
        default:
            break;
    }
    return libyuv::RotationMode::kRotate0;
}

bool contains(webrtc::VideoFrameBuffer::Type type, const rtc::ArrayView<webrtc::VideoFrameBuffer::Type>& types)
{
    return types.end() != std::find(types.begin(), types.end(), type);
}

template <class TBaseBuffer>
template <class... Types>
inline MFVideoBuffer<TBaseBuffer>::
    MFVideoBuffer(int width, int height, BYTE* buffer, DWORD actualBufferLen,
                  DWORD totalBufferLen, const CComPtr<IMediaSample>& data, Types&&... args)
    : TBaseBuffer(std::forward<Types>(args)...)
    , _width(width)
    , _height(height)
    , _data(data)
    , _buffer(buffer)
    , _actualBufferLen(actualBufferLen)
    , _totalBufferLen(totalBufferLen)
{
    assert(_width > 0);
    assert(_height > 0);
    assert(_data);
    assert(_buffer);
    assert(_actualBufferLen > 0UL);
    assert(_totalBufferLen >= _actualBufferLen);
}

MFI420VideoBuffer::MFI420VideoBuffer(int width, int height, BYTE* buffer,
                                     DWORD actualBufferLen, DWORD totalBufferLen,
                                     const CComPtr<IMediaSample>& data,
                                     VideoFrameBufferPool framesPool)
    : Base(width, height, buffer, actualBufferLen, totalBufferLen, data)
    , _framesPool(std::move(framesPool))
{
}

const uint8_t* MFI420VideoBuffer::DataU() const
{
    return Base::buffer() + StrideY() * Base::height();
}

const uint8_t* MFI420VideoBuffer::DataV() const
{
    const auto h = Base::height();
    return Base::buffer() + StrideY() * h + StrideU() * ((h + 1) / 2);
}

MFNV12VideoBuffer::MFNV12VideoBuffer(int width, int height, BYTE* buffer,
                                     DWORD actualBufferLen, DWORD totalBufferLen,
                                     const CComPtr<IMediaSample>& data,
                                     VideoFrameBufferPool framesPool)
    : Base(width, height, buffer, actualBufferLen, totalBufferLen, data, std::move(framesPool))
{
}

const uint8_t* MFNV12VideoBuffer::DataY() const
{
    return NV12VideoFrameBuffer::nv12DataY(Base::buffer());
}

const uint8_t* MFNV12VideoBuffer::DataUV() const
{
    return NV12VideoFrameBuffer::nv12DataUV(Base::buffer(), Base::width(), Base::height());
}

MFNativeBuffer::MFNativeBuffer(int width, int height, BYTE* buffer,
                               DWORD actualBufferLen, DWORD totalBufferLen,
                               const CComPtr<IMediaSample>& data,
                               VideoFrameType bufferType,
                               webrtc::VideoRotation rotation,
                               VideoFrameBufferPool framesPool)
    : Base(targetWidth(width, std::abs(height), rotation), 
          targetHeight(width, std::abs(height), rotation), 
          buffer, actualBufferLen, totalBufferLen, data, std::move(framesPool))
    , _originalWidth(width)
    , _originalHeight(height)
    , _rotation(rotation)
    , _bufferType(bufferType)
{    
}

int MFNativeBuffer::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return actualBufferLen();
    }
    return 0;
}

const std::byte* MFNativeBuffer::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(buffer());
    }
    return nullptr;
}

int MFNativeBuffer::targetWidth(int width, int height, webrtc::VideoRotation rotation)
{
    swapIfRotated(rotation, width, height);
    return width;
}

int MFNativeBuffer::targetHeight(int width, int height, webrtc::VideoRotation rotation)
{
    swapIfRotated(rotation, width, height);
    return std::abs(height);
}

void MFNativeBuffer::swapIfRotated(webrtc::VideoRotation rotation, int& width, int& height)
{
    switch (rotation) {
        case webrtc::VideoRotation::kVideoRotation_90:
        case webrtc::VideoRotation::kVideoRotation_270:
            std::swap(width, height);
            break;
        default:
            break;
    }
}

rtc::scoped_refptr<webrtc::I420BufferInterface> MFNativeBuffer::convertToI420() const
{
    const auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::ConvertToI420(buffer(), actualBufferLen(), 
                                          i420->MutableDataY(),
                                          i420->StrideY(), i420->MutableDataU(),
                                          i420->StrideU(), i420->MutableDataV(),
                                          i420->StrideV(), 0, 0, // no cropping
                                          _originalWidth, _originalHeight,
                                          i420->width(), i420->height(),
                                          fromVideoFrameRotation(_rotation),
                                          fourcc(_bufferType))) {
        return i420;
    }
    return {};
}

}