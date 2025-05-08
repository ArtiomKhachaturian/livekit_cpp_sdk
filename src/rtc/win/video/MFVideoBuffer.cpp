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
#include "MFVideoBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "NativeVideoFrameBuffer.h"
#include "LibyuvImport.h"
#include "VideoUtils.h"
#include <api/make_ref_counted.h>
#include <modules/video_capture/video_capture_defines.h>
#include <cassert>
#include <type_traits>

namespace 
{

using namespace LiveKitCpp;

libyuv::RotationMode fromVideoFrameRotation(webrtc::VideoRotation rotation);
bool contains(webrtc::VideoFrameBuffer::Type type, const rtc::ArrayView<webrtc::VideoFrameBuffer::Type>& types);

template <class TBaseBuffer, class TMediaData, class TBaseAccessor>
class MFVideoBufferImpl : public TBaseBuffer, public TBaseAccessor
{
public:
    ~MFVideoBufferImpl() override;
    // impl. of VideoFrameBuffer
    int width() const final { return _width; }
    int height() const final { return _height; }
    // impl. of TBaseAccessor
    const BYTE* buffer() const final { return _buffer; }
    DWORD actualBufferLen() const final { return _actualBufferLen; }
    DWORD totalBufferLen() const final { return _totalBufferLen; }
protected:
    template <class... Types>
    MFVideoBufferImpl(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      TMediaData data,
                      Types&&... args);
private:
    const int _width;
    const int _height;
    const TMediaData _data;
    BYTE* const _buffer;
    const DWORD _actualBufferLen;
    const DWORD _totalBufferLen;
};

template <class TMediaData, class TBaseAccessor>
class MFI420VideoBuffer : public MFVideoBufferImpl<webrtc::I420BufferInterface, TMediaData, TBaseAccessor>
{
    using Base = MFVideoBufferImpl<webrtc::I420BufferInterface, TMediaData, TBaseAccessor>;
public:
    MFI420VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      TMediaData data,
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

template <class TMediaData, class TBaseAccessor>
class MFNV12VideoBuffer : public MFVideoBufferImpl<NV12VideoFrameBuffer, TMediaData, TBaseAccessor>
{
    using Base = MFVideoBufferImpl<NV12VideoFrameBuffer, TMediaData, TBaseAccessor>;
public:
    MFNV12VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      TMediaData data,
                      VideoFrameBufferPool framesPool);
    // impl. of NV12VideoFrameBuffer
    const uint8_t* DataY() const final;
    const uint8_t* DataUV() const final;
};

template <class TMediaData, class TBaseAccessor>
class MFNativeBuffer : public MFVideoBufferImpl<VideoFrameBuffer<NativeVideoFrameBuffer>, TMediaData, TBaseAccessor>
{
    using Base = MFVideoBufferImpl<VideoFrameBuffer<NativeVideoFrameBuffer>, TMediaData, TBaseAccessor>;
public:
    MFNativeBuffer(int width, int height, BYTE* buffer,
                   DWORD actualBufferLen, DWORD totalBufferLen,
                   TMediaData data,
                   VideoFrameType bufferType,
                   webrtc::VideoRotation rotation,
                   VideoFrameBufferPool framesPool);
    // impl. of NativeVideoFrameBuffer
    VideoFrameType nativeType() const final { return _bufferType; }
    int stride(size_t planeIndex) const final;
    const std::byte* data(size_t planeIndex) const final;
    int dataSize() const final;
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

using MFSampleI420Buffer = MFI420VideoBuffer<CComPtr<IMediaSample>, MFMediaSampleBuffer>;
using MFSampleN12Buffer = MFNV12VideoBuffer<CComPtr<IMediaSample>, MFMediaSampleBuffer>;
using MFSampleNativeBuffer = MFNativeBuffer<CComPtr<IMediaSample>, MFMediaSampleBuffer>;

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
                    return webrtc::make_ref_counted<MFSampleI420Buffer>(width, absHeight,
                                                                        buffer, actualBufferLen, 
                                                                        totalBufferLen, sample, framesPool);
                case VideoFrameType::NV12:
                    return webrtc::make_ref_counted<MFSampleN12Buffer>(width, absHeight,
                                                                       buffer, actualBufferLen,
                                                                       totalBufferLen, sample, framesPool);
                default:
                    break;
            }
            return webrtc::make_ref_counted<MFSampleNativeBuffer>(width, absHeight,
                                                                  buffer, actualBufferLen,
                                                                  totalBufferLen, sample, bufferType, 
                                                                  rotation, framesPool);
        }
    }
    return {};
}

const MFMediaSampleBuffer* MFMediaSampleBuffer::sampleBuffer(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buffer)
{
    return dynamic_cast<const MFMediaSampleBuffer*>(buffer.get());
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> MFMediaBuffer::
    create(int width, int height, MFMediaBufferLocker mediaBufferLocker,
           const VideoFrameBufferPool& framesPool)
{
    if (width > 0 && height > 0 && mediaBufferLocker) {
        using MFMediaN12Buffer = MFNV12VideoBuffer<MFMediaBufferLocker, MFMediaBuffer>;
        const auto buffer = mediaBufferLocker.dataBuffer();
        const auto actualBufferLen = mediaBufferLocker.currentLen();
        const auto totalBufferLen = mediaBufferLocker.maxLen();
        return webrtc::make_ref_counted<MFMediaN12Buffer>(width, height,
                                                          buffer, actualBufferLen,
                                                          totalBufferLen, 
                                                          std::move(mediaBufferLocker),
                                                          framesPool);
    }
    return {};
}

const MFMediaBuffer* MFMediaBuffer::mediaBuffer(const rtc::scoped_refptr<webrtc::NV12BufferInterface>& buffer)
{
    return dynamic_cast<const MFMediaBuffer*>(buffer.get());
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

template <class TBaseBuffer, class TMediaData, class TBaseAccessor>
template <class... Types>
inline MFVideoBufferImpl<TBaseBuffer, TMediaData, TBaseAccessor>::
    MFVideoBufferImpl(int width, int height, BYTE* buffer, DWORD actualBufferLen,
                      DWORD totalBufferLen, TMediaData data, Types&&... args)
    : TBaseBuffer(std::forward<Types>(args)...)
    , _width(width)
    , _height(height)
    , _data(std::move(data))
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

template <class TBaseBuffer, class TMediaData, class TBaseAccessor>
inline MFVideoBufferImpl<TBaseBuffer, TMediaData, TBaseAccessor>::~MFVideoBufferImpl()
{
    if constexpr (std::is_same<IMFMediaBuffer, TMediaData>::value) {
        _data->Unlock();
    }
}

template <class TMediaData, class TBaseAccessor>
inline MFI420VideoBuffer<TMediaData, TBaseAccessor>::
    MFI420VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      TMediaData data,
                      VideoFrameBufferPool framesPool)
    : Base(width, height, buffer, actualBufferLen, totalBufferLen, std::move(data))
    , _framesPool(std::move(framesPool))
{
}

template <class TMediaData, class TBaseAccessor>
inline const uint8_t* MFI420VideoBuffer<TMediaData, TBaseAccessor>::DataU() const
{
    return Base::buffer() + StrideY() * Base::height();
}

template <class TMediaData, class TBaseAccessor>
inline const uint8_t* MFI420VideoBuffer<TMediaData, TBaseAccessor>::DataV() const
{
    const auto h = Base::height();
    return Base::buffer() + StrideY() * h + StrideU() * ((h + 1) / 2);
}

template <class TMediaData, class TBaseAccessor>
inline MFNV12VideoBuffer<TMediaData, TBaseAccessor>::
    MFNV12VideoBuffer(int width, int height, BYTE* buffer,
                      DWORD actualBufferLen, DWORD totalBufferLen,
                      TMediaData data,
                      VideoFrameBufferPool framesPool)
    : Base(width, height, buffer, actualBufferLen, totalBufferLen, std::move(data), std::move(framesPool))
{
}

template <class TMediaData, class TBaseAccessor>
const uint8_t* MFNV12VideoBuffer<TMediaData, TBaseAccessor>::DataY() const
{
    return NV12VideoFrameBuffer::nv12DataY(Base::buffer());
}

template <class TMediaData, class TBaseAccessor>
inline const uint8_t* MFNV12VideoBuffer<TMediaData, TBaseAccessor>::DataUV() const
{
    return NV12VideoFrameBuffer::nv12DataUV(Base::buffer(), Base::width(), Base::height());
}

template <class TMediaData, class TBaseAccessor>
inline MFNativeBuffer<TMediaData, TBaseAccessor>::
    MFNativeBuffer(int width, int height, BYTE* buffer,
                   DWORD actualBufferLen, DWORD totalBufferLen,
                   TMediaData data,
                   VideoFrameType bufferType,
                   webrtc::VideoRotation rotation,
                   VideoFrameBufferPool framesPool)
    : Base(targetWidth(width, std::abs(height), rotation), 
          targetHeight(width, std::abs(height), rotation), 
          buffer, actualBufferLen, totalBufferLen, std::move(data), std::move(framesPool))
    , _originalWidth(width)
    , _originalHeight(height)
    , _rotation(rotation)
    , _bufferType(bufferType)
{    
}

template <class TMediaData, class TBaseAccessor>
inline int MFNativeBuffer<TMediaData, TBaseAccessor>::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return Base::actualBufferLen();
    }
    return 0;
}

template <class TMediaData, class TBaseAccessor>
inline const std::byte* MFNativeBuffer<TMediaData, TBaseAccessor>::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(Base::buffer());
    }
    return nullptr;
}

template <class TMediaData, class TBaseAccessor>
inline int MFNativeBuffer<TMediaData, TBaseAccessor>::dataSize() const
{
    return Base::actualBufferLen();
}

template <class TMediaData, class TBaseAccessor>
inline int MFNativeBuffer<TMediaData, TBaseAccessor>::targetWidth(int width, 
                                                                  int height, 
                                                                  webrtc::VideoRotation rotation)
{
    swapIfRotated(rotation, width, height);
    return width;
}

template <class TMediaData, class TBaseAccessor>
inline int MFNativeBuffer<TMediaData, TBaseAccessor>::targetHeight(int width, 
                                                                   int height, 
                                                                   webrtc::VideoRotation rotation)
{
    swapIfRotated(rotation, width, height);
    return std::abs(height);
}

template <class TMediaData, class TBaseAccessor>
inline void MFNativeBuffer<TMediaData, TBaseAccessor>::swapIfRotated(webrtc::VideoRotation rotation, 
                                                                    int& width, int& height)
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

template <class TMediaData, class TBaseAccessor>
inline rtc::scoped_refptr<webrtc::I420BufferInterface> 
    MFNativeBuffer<TMediaData, TBaseAccessor>::convertToI420() const
{
    const auto i420 = Base::createI420(Base::width(), Base::height());
    if (i420 && 0 == libyuv::ConvertToI420(Base::buffer(), Base::actualBufferLen(),
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