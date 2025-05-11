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
#ifdef USE_PLATFORM_ENCODERS
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

class VTEncoderSourceFrame::Planes
{
public:
    bool set(const NativeVideoFrameBuffer* native);
    bool set(const webrtc::NV12BufferInterface* nv12);
    bool set(const webrtc::scoped_refptr<webrtc::NV12BufferInterface>& nv12);
    CVPixelBufferRef create(CompletionStatus* status = nullptr);
private:
    static void releaseCallback(void* frameRef,
                                const void* data,
                                size_t /*size*/,
                                size_t /*numPlanes*/,
                                const void* /*planes*/[]);
private:
    void* _ptrs[_maxPlanes] = {};
    size_t _bytesPerRow[_maxPlanes] = {};
    size_t _widths[_maxPlanes] = {};
    size_t _heights[_maxPlanes] = {};
    size_t _numPlanes = 0U;
    size_t _dataSize = 0U;
    OSType _format = 0;
    const webrtc::VideoFrameBuffer* _attachedBuffer;
};

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

CompletionStatusOr<VTEncoderSourceFrame> VTEncoderSourceFrame::create(const webrtc::VideoFrame& frame,
                                                                      const VideoFrameBufferPool& framesPool)
{
    auto pixelBuffer = convertToPixelBuffer(frame, framesPool);
    if (pixelBuffer) {
        return VTEncoderSourceFrame(frame, pixelBuffer.moveValue());
    }
    return pixelBuffer.moveStatus();
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
        CVPixelBufferRef mappedBuffer = CoreVideoPixelBuffer::pixelBuffer(buffer, true); // retain
        if (mappedBuffer) {
            return CVPixelBufferAutoRelease(mappedBuffer);
        }
        // build arrays for each plane's data pointer, dimensions and byte alignment
        Planes pixmapPlanes;
        bool ok = false;
        switch (buffer->type()) {
            case webrtc::VideoFrameBuffer::Type::kNative:
                ok = pixmapPlanes.set(dynamic_cast<const NativeVideoFrameBuffer*>(buffer.get()));
                break;
            default:
                break;
        }
        if (!ok) {
            ok = pixmapPlanes.set(NV12VideoFrameBuffer::toNV12(buffer, framesPool));
        }
        if (ok) {
            CompletionStatus status;
            mappedBuffer = pixmapPlanes.create(&status);
            if (mappedBuffer) {
                return CVPixelBufferAutoRelease(mappedBuffer);
            }
            return status;
        }
    }
    return COMPLETION_STATUS(kCMSampleBufferError_BufferNotReady);
}

bool VTEncoderSourceFrame::Planes::set(const NativeVideoFrameBuffer* native)
{
    if (native) {
        const auto format = mapToPixelFormat(native->nativeType());
        if (format) {
            const auto numPlanes = planesCount(native->nativeType());
            if (numPlanes > 0U && numPlanes <= _maxPlanes) {
                _format = format;
                _numPlanes = numPlanes;
                _dataSize = webrtc::CalcBufferSize(map(native->nativeType()),
                                                   native->width(), native->height());
                for (size_t i = 0U; i < numPlanes; ++i) {
                    _ptrs[i] = cast(native->data(i));
                    _bytesPerRow[i] = native->stride(i);
                    _widths[i] = native->width();
                    _heights[i] = native->height();
                }
                _attachedBuffer = native;
                return true;
            }
        }
    }
    return false;
}

bool VTEncoderSourceFrame::Planes::set(const webrtc::NV12BufferInterface* nv12)
{
    if (nv12) {
        _numPlanes = 2U;
        _dataSize = webrtc::CalcBufferSize(webrtc::VideoType::kNV12, nv12->width(), nv12->height());
        _ptrs[0] = cast(nv12->DataY());
        _ptrs[1] = cast(nv12->DataUV());
        _bytesPerRow[0] = nv12->StrideY();
        _bytesPerRow[1] = nv12->StrideUV();
        _widths[0] = _widths[1] = nv12->ChromaWidth();
        _heights[0] = _heights[1] = nv12->ChromaHeight();
        _format = formatNV12Full();
        _attachedBuffer = nv12;
        return true;
    }
    return false;
}

bool VTEncoderSourceFrame::Planes::set(const webrtc::scoped_refptr<webrtc::NV12BufferInterface>& nv12)
{
    return set(nv12.get());
}

CVPixelBufferRef VTEncoderSourceFrame::Planes::create(CompletionStatus* error)
{
    CVPixelBufferRef mappedBuffer = nullptr;
    if (_numPlanes && _attachedBuffer) {
        // CVPixelBufferCreateWithPlanarBytes needs a dummy plane descriptor or the
        // release callback will not execute. The descriptor is freed in the callback.
        void* dummy = std::calloc(1, std::max(sizeof(CVPlanarPixelBufferInfo_YCbCrPlanar),
                                              sizeof(CVPlanarPixelBufferInfo_YCbCrBiPlanar)));
        const auto status = COMPLETION_STATUS(CVPixelBufferCreateWithPlanarBytes(kCFAllocatorDefault,           // allocator
                                                                                 _attachedBuffer->width(),       // width
                                                                                 _attachedBuffer->height(),      // height
                                                                                 _format,                        // pixelFormatType
                                                                                 dummy,                         // dataPtr (pointer to a plane descriptor block)
                                                                                 _dataSize,                      // dataSize
                                                                                 _numPlanes,                     // numberOfPlanes
                                                                                 _ptrs,                     // planeBaseAddress
                                                                                 _widths,                   // planeWidth
                                                                                 _heights,                  // planeHeight
                                                                                 _bytesPerRow,              // planeBytesPerRow
                                                                                 &releaseCallback,        // releaseCallback
                                                                                 (void*)_attachedBuffer,   // releaseRefCon
                                                                                 nullptr,                       // pixelBufferAttributes
                                                                                 &mappedBuffer));                // pixelBufferOut
        if (status) {
            _attachedBuffer->AddRef();
        }
        if (error) {
            *error = std::move(status);
        }
    }
    else if (error) {
        *error = COMPLETION_STATUS(kVTParameterErr);
    }
    return mappedBuffer;
}

// CVPixelBuffer release callback. See |GetCvPixelBufferRepresentation()|.
void VTEncoderSourceFrame::Planes::releaseCallback(void* frameRef,
                                                   const void* data,
                                                   size_t /*size*/,
                                                   size_t /*numPlanes*/,
                                                   const void* /*planes*/[])
{
    std::free(const_cast<void*>(data));
    reinterpret_cast<const webrtc::VideoFrameBuffer*>(frameRef)->Release();
}

} // namespace LiveKitCpp
#endif
