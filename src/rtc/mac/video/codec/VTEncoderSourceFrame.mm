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
#include "VTEncoderSourceFrame.h"
#include "CoreVideoPixelBuffer.h"
#include "LibyuvImport.h"
#include "NativeVideoFrameBuffer.h"
#include "NV12VideoFrameBuffer.h"
#include "RtcUtils.h"
#include "VideoUtils.h"
#include <api/video/video_frame.h>

namespace {

template <typename T = uint8_t>
inline void* cast(const T* addr) {
    return reinterpret_cast<void*>(const_cast<T*>(addr));
}

// CVPixelBuffer release callback. See |GetCvPixelBufferRepresentation()|.
inline void cvPixelBufferReleaseCallback(void* frameRef,
                                         const void* data,
                                         size_t /*size*/,
                                         size_t /*numPlanes*/,
                                         const void* /*planes*/[])
{
    std::free(const_cast<void*>(data));
    reinterpret_cast<webrtc::VideoFrameBuffer*>(frameRef)->Release();
}

inline OSType mapToPixelFormat(LiveKitCpp::VideoFrameType type)
{
    switch (type) {
        case LiveKitCpp::VideoFrameType::RGB24:
            return LiveKitCpp::formatRGB24();
        case LiveKitCpp::VideoFrameType::BGR24:
            return LiveKitCpp::formatBGR24();
        case LiveKitCpp::VideoFrameType::BGRA32:
            return LiveKitCpp::formatBGRA32();
        case LiveKitCpp::VideoFrameType::ARGB32:
            return LiveKitCpp::formatARGB32();
        case LiveKitCpp::VideoFrameType::RGBA32:
            return LiveKitCpp::formatRGBA32();
        case LiveKitCpp::VideoFrameType::ABGR32:
            return LiveKitCpp::formatABGR32();
        case LiveKitCpp::VideoFrameType::NV12:
            return LiveKitCpp::formatNV12Full();
        case LiveKitCpp::VideoFrameType::I420:
            return LiveKitCpp::formatI420();
        default:
            break;
    }
    return 0;
}

}

namespace LiveKitCpp
{

VTEncoderSourceFrame::VTEncoderSourceFrame(const webrtc::VideoFrame& frame,
                                           CVPixelBufferAutoRelease mappedBuffer)
    : _frame(frame)
    , _mappedBuffer(std::move(mappedBuffer))
{
}

VTEncoderSourceFrame::~VTEncoderSourceFrame()
{
}

OSType VTEncoderSourceFrame::pixelFormat() const
{
    return _mappedBuffer.pixelFormat();
}

webrtc::RTCErrorOr<VTEncoderSourceFrame> VTEncoderSourceFrame::create(const webrtc::VideoFrame& frame,
                                                                      const VideoFrameBufferPool& framesPool)
{
    auto pixelBuffer = convertToPixelBuffer(frame, framesPool);
    if (pixelBuffer.ok()) {
        return VTEncoderSourceFrame(frame, pixelBuffer.MoveValue());
    }
    return pixelBuffer.MoveError();
}

void VTEncoderSourceFrame::setStartTimestamp(int64_t timestampMs)
{
    _startTimestampMs = timestampMs;
}

void VTEncoderSourceFrame::setFinishTimestamp(int64_t timestampMs)
{
    _finishTimestampMs = timestampMs;
}

int64_t VTEncoderSourceFrame::currentTimestampMs()
{
    return webrtc::TimeMillis();
}

VTEncoderSourceFrame::PixelBuffer VTEncoderSourceFrame::
    convertToPixelBuffer(const webrtc::VideoFrame& frame, const VideoFrameBufferPool& framesPool)
{
    if (const auto buffer = frame.video_frame_buffer()) {
        auto mappedBuffer = CoreVideoPixelBuffer::pixelBuffer(buffer, true); // retain
        if (!mappedBuffer) {
            // build arrays for each plane's data pointer, dimensions and byte alignment
            static thread_local void* planePtrs[_maxPlanes] = {};
            static thread_local size_t planeBytesPerRow[_maxPlanes] = {};
            static thread_local size_t planeWidths[_maxPlanes] = {};
            static thread_local size_t planeHeights[_maxPlanes] = {};
            size_t numPlanes = 0U, dataSize = 0U;
            OSType format = 0;
            webrtc::scoped_refptr<const webrtc::VideoFrameBuffer> attachedBuffer = buffer;
            webrtc::scoped_refptr<const webrtc::NV12BufferInterface> mappedNV12;
            switch (buffer->type()) {
                case webrtc::VideoFrameBuffer::Type::kNative:
                    if (const auto native = dynamic_cast<const NativeVideoFrameBuffer*>(buffer.get())) {
                        format = mapToPixelFormat(native->nativeType());
                        if (format) {
                            numPlanes = planesCount(native->nativeType());
                            if (numPlanes > 0U && numPlanes <= _maxPlanes) {
                                dataSize = native->dataSize();
                                for (size_t i = 0U; i < numPlanes; ++i) {
                                    planePtrs[i] = cast(native->data(i));
                                    planeBytesPerRow[i] = native->stride(i);
                                    planeWidths[i] = native->width();
                                    planeHeights[i] = native->height();
                                }
                                break;
                            }
                        }
                    }
                default:
                    attachedBuffer = mappedNV12 = NV12VideoFrameBuffer::toNV12(buffer, framesPool);
                    break;
            }
            if (mappedNV12) {
                numPlanes = 2U;
                dataSize = webrtc::CalcBufferSize(webrtc::VideoType::kNV12, buffer->width(), buffer->height());
                planePtrs[0] = cast(mappedNV12->DataY());
                planePtrs[1] = cast(mappedNV12->DataUV());
                planeBytesPerRow[0] = mappedNV12->StrideY();
                planeBytesPerRow[1] = mappedNV12->StrideUV();
                planeWidths[0] = planeWidths[1] = mappedNV12->ChromaWidth();
                planeHeights[0] = planeHeights[1] = mappedNV12->ChromaHeight();
                format = formatNV12Full();
            }
            if (numPlanes > 0U) {
                // CVPixelBufferCreateWithPlanarBytes needs a dummy plane descriptor or the
                // release callback will not execute. The descriptor is freed in the callback.
                void* dummy = std::calloc(1, std::max(sizeof(CVPlanarPixelBufferInfo_YCbCrPlanar),
                                                      sizeof(CVPlanarPixelBufferInfo_YCbCrBiPlanar)));
                const auto status = CVPixelBufferCreateWithPlanarBytes(kCFAllocatorDefault,           // allocator
                                                                       attachedBuffer->width(),       // width
                                                                       attachedBuffer->height(),      // height
                                                                       format,                        // pixelFormatType
                                                                       dummy,                         // dataPtr (pointer to a plane descriptor block)
                                                                       dataSize,                      // dataSize
                                                                       numPlanes,                     // numberOfPlanes
                                                                       planePtrs,                     // planeBaseAddress
                                                                       planeWidths,                   // planeWidth
                                                                       planeHeights,                  // planeHeight
                                                                       planeBytesPerRow,              // planeBytesPerRow
                                                                       &cvPixelBufferReleaseCallback, // releaseCallback
                                                                       (void*)attachedBuffer.get(),   // releaseRefCon
                                                                       nullptr,                       // pixelBufferAttributes
                                                                       &mappedBuffer);                // pixelBufferOut
                if (noErr == status) {
                    attachedBuffer->AddRef();
                    return CVPixelBufferAutoRelease(mappedBuffer);
                }
                return toRtcError(status);
            }
        }
        return toRtcError(kCMSampleBufferError_BufferNotReady, webrtc::RTCErrorType::INVALID_PARAMETER);
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_PARAMETER);
}

} // namespace darkmatter::rtc
