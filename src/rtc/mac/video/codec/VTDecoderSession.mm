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
#include "VTDecoderSession.h"
#include "VTSessionPipeline.h"
#include "VTDecoderSessionCallback.h"
#include "VideoDecoder.h"
#include "CoreVideoPixelBuffer.h"
#include "Utils.h"
#include <api/video/color_space.h>

namespace
{

inline CFStringRef latencyKey(bool realtime) {
    return realtime ? kVTDecompressionPropertyKey_RealTime : kVTDecompressionPropertyKey_MaximizePowerEfficiency;
}

struct EncodedData
{
    const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> _encodedBuffer;
    const uint32_t _timestamp;
    std::optional<uint8_t> _qp;
    std::optional<webrtc::ColorSpace> _colorspace;
    EncodedData(const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& encodedBuffer,
                uint32_t timestamp, int qp = -1, const webrtc::ColorSpace* colorspace = nullptr);
};

}

namespace LiveKitCpp
{

class VTDecoderSession::DecodePipeline : public VTSessionPipeline<VTDecoderSessionCallback>
{
public:
    DecodePipeline(CMVideoCodecType codecType,
                   int32_t width, int32_t height,
                   VTDecoderSessionCallback* callback = nullptr,
                   VideoFrameBufferPool framesPool = {});
    OSStatus input(VTDecompressionSessionRef session,
                   CFAutoRelease<CMSampleBufferRef> encodedBufferData,
                   const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& encodedBuffer,
                   uint32_t timestamp, VTDecodeInfoFlags* infoFlags = nullptr,
                   int qp = -1, const webrtc::ColorSpace* colorSpace = nullptr);
    static void output(void* pipeline, void *params, OSStatus status,
                       VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer,
                       CMTime timestamp, CMTime duration);
private:
    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> createBuffer(CVImageBufferRef imageBuffer) const;
    void adjustPresentationTimestamp(CMSampleBufferRef encodedBuffer, uint32_t timestamp) const;
private:
    const VideoFrameBufferPool _framesPool;
};

VTDecoderSession::VTDecoderSession(std::unique_ptr<DecodePipeline> pipeline,
                                   CFAutoRelease<CMVideoFormatDescriptionRef> format,
                                   VTDecompressionSessionRef sessionRef,
                                   bool hardwareAccelerated)
    : BaseClass(std::move(format), sessionRef, hardwareAccelerated)
    , _pipeline(std::move(pipeline))
{
}

VTDecoderSession::VTDecoderSession() = default;

VTDecoderSession::VTDecoderSession(VTDecoderSession&&) = default;

VTDecoderSession::~VTDecoderSession()
{
    if (valid()) {
        VTDecompressionSessionInvalidate(sessionRef());
    }
}

VTDecoderSession& VTDecoderSession::operator = (VTDecoderSession&&) = default;

webrtc::RTCErrorOr<VTDecoderSession> VTDecoderSession::
    create(bool hardwareAccelerated, CMVideoCodecType codecType,
           CFAutoRelease<CMVideoFormatDescriptionRef> format, OSType outputPixelFormat,
           int numberOfCores, VTDecoderSessionCallback* callback,
           VideoFrameBufferPool framesPool, bool realtime)
{
    if (format) {
        const auto dimensions = CMVideoFormatDescriptionGetDimensions(format);
        if (const auto imageAttrs = sourceImageAttributes(outputPixelFormat, dimensions.width, dimensions.height)) {
            CFMutableDictionaryRefAutoRelease decoderConfig = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                                        2,  // capacity
                                                                                        &kCFTypeDictionaryKeyCallBacks,
                                                                                        &kCFTypeDictionaryValueCallBacks);
            CFDictionarySetValue(decoderConfig, kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder,
                                 hardwareAccelerated ? kCFBooleanTrue : kCFBooleanFalse);
            if (!hardwareAccelerated) {
                numberOfCores = VideoDecoder::maxDecodingThreads(dimensions.width, dimensions.height, numberOfCores);
                if (numberOfCores > 0) {
                    CFDictionarySetValue(decoderConfig, kVTDecompressionPropertyKey_ThreadCount, createCFNumber(numberOfCores));
                }
            }
            auto pipeline = std::make_unique<DecodePipeline>(codecType,
                                                             dimensions.width,
                                                             dimensions.height,
                                                             callback,
                                                             std::move(framesPool));
            VTDecompressionSessionRef session = nullptr;
            VTDecompressionOutputCallbackRecord record = { DecodePipeline::output, pipeline.get() };
            auto status = VTDecompressionSessionCreate(kCFAllocatorDefault, format, decoderConfig,
                                                       imageAttrs, &record, &session);
            if (noErr == status) {
                CFBooleanRef hwacclEnabled = nil;
                hardwareAccelerated = noErr == VTSessionCopyProperty(session,
                                                                     kVTDecompressionPropertyKey_UsingHardwareAcceleratedVideoDecoder,
                                                                     nil, &hwacclEnabled) && CFBooleanGetValue(hwacclEnabled);
                // enable low-latency mode
                VTSessionSetProperty(session, latencyKey(realtime), kCFBooleanTrue);
                return VTDecoderSession(std::move(pipeline), std::move(format), session, hardwareAccelerated);
            }
            return toRtcError(status);
        }
    }
    return toRtcError(kVTParameterErr, webrtc::RTCErrorType::INVALID_PARAMETER);
}

webrtc::RTCError VTDecoderSession::waitForAsynchronousFrames()
{
    if (valid()) {
        return toRtcError(VTDecompressionSessionWaitForAsynchronousFrames(sessionRef()));
    }
    return toRtcError(kVTInvalidSessionErr, webrtc::RTCErrorType::INVALID_STATE);
}

webrtc::RTCError VTDecoderSession::setOutputPoolRequestedMinimumBufferCount(int bufferPoolSize)
{
    return setProperty(kVTDecompressionPropertyKey_OutputPoolRequestedMinimumBufferCount, bufferPoolSize);
}

OSStatus VTDecoderSession::decompress(CMSampleBufferRef encodedBufferData,
                                      const webrtc::EncodedImage& image,
                                      VTDecodeInfoFlags* infoFlags) const
{
    if (_pipeline) {
        if (const auto encodedBuffer = image.GetEncodedData()) {
            return _pipeline->input(sessionRef(), encodedBufferData, encodedBuffer, image.RtpTimestamp(),
                                    infoFlags, image.qp_, image.ColorSpace());
        }
        return kVTParameterErr;
    }
    return kVTInvalidSessionErr;
}

uint64_t VTDecoderSession::pendingFramesCount() const
{
    return _pipeline ? _pipeline->pendingFramesCount() : 0ULL;
}

OSStatus VTDecoderSession::lastOutputStatus() const
{
    if (_pipeline) {
        return _pipeline->lastOutputStatus();
    }
    return kVTInvalidSessionErr;
}

CFDictionaryRefAutoRelease VTDecoderSession::sourceImageAttributes(OSType pixelFormat, int32_t width, int32_t height)
{
    if (width > 0 && height > 0) {
        const auto pixelFormatNumber = createCFNumber(static_cast<int64_t>(pixelFormat), kCFNumberLongType);
        return createImageBufferAttributes(pixelFormatNumber, width, height);
    }
    return nullptr;
}

VTDecoderSession::DecodePipeline::DecodePipeline(CMVideoCodecType codecType,
                                                 int32_t width, int32_t height,
                                                 VTDecoderSessionCallback* callback,
                                                 VideoFrameBufferPool framesPool)
    :  VTSessionPipeline<VTDecoderSessionCallback>(codecType, callback)
    , _framesPool(std::move(framesPool))
{
}

OSStatus VTDecoderSession::DecodePipeline::input(VTDecompressionSessionRef session,
                                                 CFAutoRelease<CMSampleBufferRef> encodedBufferData,
                                                 const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& encodedBuffer,
                                                 uint32_t timestamp, VTDecodeInfoFlags* infoFlags,
                                                 int qp, const webrtc::ColorSpace* colorspace)
{
    if (session && encodedBuffer) {
        adjustPresentationTimestamp(encodedBufferData, timestamp);
        auto encodedData = std::make_unique<EncodedData>(encodedBuffer, timestamp, qp, colorspace);
        beginInput();
        return endInput(VTDecompressionSessionDecodeFrame(session, encodedBufferData,
                                                          kVTDecodeFrame_EnableAsynchronousDecompression,
                                                          encodedData.release(), infoFlags));
    }
    return kVTParameterErr;
}

void VTDecoderSession::DecodePipeline::output(void* pipeline, void *params, OSStatus status,
                                              VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer,
                                              CMTime timestamp, CMTime duration)
{
    std::unique_ptr<EncodedData> encodedData(reinterpret_cast<EncodedData*>(params));
    if (auto selfRef = reinterpret_cast<DecodePipeline*>(pipeline)) {
        const auto statusChanged = selfRef->endOutput(status);
        if (noErr == status) {
            webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer;
            if (!testFlag<kVTDecodeInfo_FrameDropped>(infoFlags)) {
                buffer = selfRef->createBuffer(imageBuffer);
            }
            selfRef->callback(&VTDecoderSessionCallback::onDecodedImage, timestamp,
                              duration, infoFlags,
                              encodedData->_timestamp, buffer,
                              std::move(encodedData->_qp),
                              std::move(encodedData->_colorspace));
        }
        else if (statusChanged) {
            selfRef->callback(&VTDecoderSessionCallback::onError, status, true);
        }
    }
}

webrtc::scoped_refptr<webrtc::VideoFrameBuffer> VTDecoderSession::DecodePipeline::createBuffer(CVImageBufferRef imageBuffer) const
{
    return CoreVideoPixelBuffer::create(imageBuffer, _framesPool);
}

void VTDecoderSession::DecodePipeline::adjustPresentationTimestamp(CMSampleBufferRef encodedBuffer, uint32_t timestamp) const
{
    if (encodedBuffer && 0 == CMTimeCompare(kCMTimeInvalid, CMSampleBufferGetOutputPresentationTimeStamp(encodedBuffer))) {
        const auto presentationTimeStamp = CMTimeMake(timestamp, 1000);
        const auto status = CMSampleBufferSetOutputPresentationTimeStamp(encodedBuffer, presentationTimeStamp);
        if (!status) {
            callback(&VTDecoderSessionCallback::onError, status, false);
        }
    }
}

} // namespace LiveKitCpp

namespace
{

EncodedData::EncodedData(const webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface>& encodedBuffer,
                         uint32_t timestamp, int qp, const webrtc::ColorSpace* colorspace)
    : _encodedBuffer(encodedBuffer)
    , _timestamp(timestamp)
    , _colorspace(colorspace ? std::make_optional(*colorspace) : std::nullopt)
{
    if (-1 != qp) {
        _qp = static_cast<uint8_t>(qp);
    }
}

}
