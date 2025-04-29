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
#include "MFI420VideoBuffer.h"
#include "MFNV12VideoBuffer.h"
#include "VideoUtils.h"
#include <api/make_ref_counted.h>
#include <common_video/libyuv/include/webrtc_libyuv.h>
#include <libyuv/convert.h>
#include <modules/video_capture/video_capture_defines.h>

namespace 
{

inline libyuv::RotationMode fromVideoFrameRotation(webrtc::VideoRotation rotation)
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

inline bool contains(webrtc::VideoFrameBuffer::Type type, const rtc::ArrayView<webrtc::VideoFrameBuffer::Type>& types)
{
    return types.end() != std::find(types.begin(), types.end(), type);
}

} // namespace

namespace LiveKitCpp 
{

MFMediaSampleBuffer::MFMediaSampleBuffer(int width, int height, VideoFrameType bufferType,
                                         BYTE* buffer, DWORD actualBufferLen, DWORD totalBufferLen,
                                         const CComPtr<IMediaSample>& sample,
                                         webrtc::VideoRotation rotation,
                                         VideoFrameBufferPool framesPool)
    : BaseClass(targetWidth(width, std::abs(height), rotation), 
                targetHeight(width, std::abs(height), rotation),
                buffer, actualBufferLen, totalBufferLen, 
                sample, std::move(framesPool))
    , _originalWidth(width)
    , _originalHeight(height)
    , _rotation(rotation)
    , _bufferType(bufferType)
{
}

int MFMediaSampleBuffer::stride(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return actualBufferLen();
    }
    return 0;
}

const std::byte* MFMediaSampleBuffer::data(size_t planeIndex) const
{
    if (0U == planeIndex) {
        return reinterpret_cast<const std::byte*>(buffer());
    }
    return nullptr;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> MFMediaSampleBuffer::create(int width, int height,
                                                                         webrtc::VideoType bufferType, BYTE* buffer,
                                                                         DWORD actualBufferLen, DWORD totalBufferLen,
                                                                         const CComPtr<IMediaSample>& sample,
                                                                         webrtc::VideoRotation rotation,
                                                                         VideoFrameBufferPool framesPool)
{
    if (width > 0 && buffer && actualBufferLen && sample && webrtc::VideoType::kUnknown != bufferType) {
        // setting absolute height (in case it was negative),
        // in Windows, the image starts bottom left, instead of top left,
        // setting a negative source height, inverts the image (within LibYuv),
        // see also [translateMediaTypeToVideoCaptureCapability] function for RGB24 cases
        const int absHeight = std::abs(height);
        if (absHeight > 0) {
            if (webrtc::VideoType::kMJPEG == bufferType || actualBufferLen == webrtc::CalcBufferSize(bufferType,
                                                                                                     width,
                                                                                                     absHeight)) {
                switch (bufferType) {
                    case webrtc::VideoType::kI420:
                        return rtc::make_ref_counted<MFI420VideoBuffer<IMediaSample>>(width, height, 
                                                                                      buffer,
                                                                                      actualBufferLen, 
                                                                                      totalBufferLen,
                                                                                      sample,
                                                                                      std::move(framesPool));
                    default:
                        break;
                }
                const auto nativeType = map(bufferType);
                if (nativeType) {
                    return rtc::make_ref_counted<MFMediaSampleBuffer>(width, height,
                                                                      nativeType.value(), buffer,
                                                                      actualBufferLen,
                                                                      totalBufferLen,
                                                                      sample, rotation,
                                                                      std::move(framesPool));
                }
            }
        }
    }
    return nullptr;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer> MFMediaSampleBuffer::create(const webrtc::VideoCaptureCapability& frameInfo,
                                                                         BYTE* buffer, DWORD actualBufferLen, 
                                                                         DWORD totalBufferLen,
                                                                         const CComPtr<IMediaSample>& sample,
                                                                         webrtc::VideoRotation rotation,
                                                                         VideoFrameBufferPool framesPool)
{
    return create(frameInfo.width, frameInfo.height, frameInfo.videoType,
                  buffer, actualBufferLen, totalBufferLen, sample, rotation, std::move(framesPool));
}

rtc::scoped_refptr<webrtc::I420BufferInterface> MFMediaSampleBuffer::convertToI420() const
{
    const auto i420 = createI420(width(), height());
    if (i420 && 0 == libyuv::ConvertToI420(
                         buffer(), actualBufferLen(), i420->MutableDataY(),
                         i420->StrideY(), i420->MutableDataU(),
                         i420->StrideU(), i420->MutableDataV(),
                         i420->StrideV(), 0, 0, // no cropping
                         _originalWidth, _originalHeight,
                         i420->width(), i420->height(),
                         fromVideoFrameRotation(_rotation),
                         fourcc(_bufferType))) {
        return i420;
    }
    return nullptr;
}

int MFMediaSampleBuffer::targetWidth(int width, int height, webrtc::VideoRotation rotation)
{
    swapIfRotated(rotation, width, height);
    return width;
}

int MFMediaSampleBuffer::targetHeight(int width, int height, webrtc::VideoRotation rotation)
{
    swapIfRotated(rotation, width, height);
    return std::abs(height);
}

void MFMediaSampleBuffer::swapIfRotated(webrtc::VideoRotation rotation, int& width, int& height)
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

} // namespace LiveKitCpp