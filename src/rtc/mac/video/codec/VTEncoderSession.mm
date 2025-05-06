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
#include "VTEncoderSession.h"
#include "VTSessionPipeline.h"
#include "VTEncoderSourceFrame.h"
#include "VTEncoderSessionCallback.h"
#include "VideoUtils.h"
#include "CFMemoryPool.h"
#include "RtcUtils.h"
#include <components/video_codec/helpers.h>
#include <rtc_base/time_utils.h>

namespace LiveKitCpp
{

class VTEncoderSession::EncodePipeline : public VTSessionPipeline<VTEncoderSessionCallback>
{
public:
    EncodePipeline(CMVideoCodecType codecType,
                   VTEncoderSessionCallback* callback = nullptr);
    CompletionStatus input(VTCompressionSessionRef session,
                           VTEncoderSourceFrame sourceFrame,
                           bool forceKeyFrame, VTEncodeInfoFlags* infoFlagsOut);
    static void output(void* pipeline, void* params, OSStatus status,
                       VTEncodeInfoFlags infoFlags, CMSampleBufferRef sampleBuffer);
};

VTEncoderSession::VTEncoderSession(const std::shared_ptr<CFMemoryPool>& compressedDataAllocator,
                                   std::unique_ptr<EncodePipeline> pipeline,
                                   VTCompressionSessionRef sessionRef,
                                   bool hardwareAccelerated,
                                   int32_t width, int32_t height)
    : VTSession<OSType, VTCompressionSessionRef>(OSType(), sessionRef, hardwareAccelerated)
    , _compressedDataAllocator(compressedDataAllocator)
    , _pipeline(std::move(pipeline))
    , _width(width)
    , _height(height)
{
}

VTEncoderSession::VTEncoderSession() = default;

VTEncoderSession::VTEncoderSession(VTEncoderSession&&) = default;

VTEncoderSession& VTEncoderSession::operator = (VTEncoderSession&&) = default;

VTEncoderSession::~VTEncoderSession()
{
    if (_pipeline) {
        _pipeline->setActive(false);
    }
    if (valid()) {
        VTCompressionSessionInvalidate(sessionRef());
    }
}

CompletionStatus VTEncoderSession::setExpectedFrameRate(uint32_t frameRate)
{
    return setProperty(kVTCompressionPropertyKey_ExpectedFrameRate, frameRate);
}

CompletionStatus VTEncoderSession::setDataRateLimits(uint32_t maxBitrateBps)
{
    // that say we set data in byte/second
    @autoreleasepool {
        NSNumber* maxBitrateNumber = [NSNumber numberWithInt: maxBitrateBps];
        // that say we set data in byte/second
        const auto limits = (__bridge CFArrayRef)@[maxBitrateNumber, @8];
        return setProperty(kVTCompressionPropertyKey_DataRateLimits, limits);
    }
}

CompletionStatus VTEncoderSession::setAverageBitRate(uint32_t bitrateBps)
{
    return setProperty(kVTCompressionPropertyKey_AverageBitRate, bitrateBps);
}

CompletionStatus VTEncoderSession::setRealTime(bool realtime)
{
    return setProperty(kVTCompressionPropertyKey_RealTime, realtime);
}

CompletionStatus VTEncoderSession::setAllowFrameReordering(bool allow)
{
    return setProperty(kVTCompressionPropertyKey_AllowFrameReordering, allow);
}

CompletionStatus VTEncoderSession::setAllowTemporalCompression(bool allow)
{
    return setProperty(kVTCompressionPropertyKey_AllowTemporalCompression, allow);
}

CompletionStatus VTEncoderSession::setMaxKeyFrameInterval(int maxKeyFrameInterval)
{
    return setProperty(kVTCompressionPropertyKey_MaxKeyFrameInterval, maxKeyFrameInterval);
}

CompletionStatus VTEncoderSession::setMaxKeyFrameIntervalDuration(int maxKeyFrameIntervalDuration)
{
    return setProperty(kVTCompressionPropertyKey_MaxKeyFrameIntervalDuration, maxKeyFrameIntervalDuration);
}

CompletionStatus VTEncoderSession::setProfileLevel(CFStringRef profile)
{
    return setProperty(kVTCompressionPropertyKey_ProfileLevel, profile);
}

CompletionStatus VTEncoderSession::prepareToEncodeFrames()
{
    if (valid()) {
        return COMPLETION_STATUS(VTCompressionSessionPrepareToEncodeFrames(sessionRef()));
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

CompletionStatus VTEncoderSession::completeFrames(CMTime completeUntilPresentationTimeStamp)
{
    if (valid()) {
        return COMPLETION_STATUS(VTCompressionSessionCompleteFrames(sessionRef(), completeUntilPresentationTimeStamp));
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

bool VTEncoderSession::isCompatible(const VTEncoderSourceFrame& frame) const
{
    return width() == frame.width() && height() == frame.height();
}

CompletionStatus VTEncoderSession::compress(VTEncoderSourceFrame sourceFrame, bool forceKeyFrame,
                                            VTEncodeInfoFlags* infoFlagsOut) const
{
    if (_pipeline) {
        return _pipeline->input(sessionRef(), std::move(sourceFrame), forceKeyFrame, infoFlagsOut);
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

uint64_t VTEncoderSession::pendingFramesCount() const
{
    return _pipeline ? _pipeline->pendingFramesCount() : 0ULL;
}

CompletionStatus VTEncoderSession::lastOutputStatus() const
{
    if (_pipeline) {
        return COMPLETION_STATUS(_pipeline->lastOutputStatus());
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

CompletionStatusOr<VTEncoderSession> VTEncoderSession::create(int32_t width,
                                                              int32_t height,
                                                              CMVideoCodecType codecType,
                                                              uint32_t qpMax,
                                                              VTEncoderSessionCallback* callback,
                                                              const std::shared_ptr<CFMemoryPool>& compressedDataAllocator)
{
    if (const auto imageAttrs = sourceImageAttributes(width, height)) {
        VTCompressionSessionRef session = nullptr;
        auto pipeline = std::make_unique<EncodePipeline>(codecType, callback);
        const auto dataAllocator = compressedDataAllocator ? compressedDataAllocator->allocator() : nullptr;
        const auto specs = encoderSpecification(width, height);
        if (specs && qpMax > 0U) {
            // not sure that this key is supported, according to https://trac.ffmpeg.org/ticket/5357:
            // "This isn't supported with H.264. It's normally used for JPEG encoding."
            qpMax = bound<uint32_t>(0U, qpMax, 100U);
            CFDictionarySetValue(specs, kVTCompressionPropertyKey_Quality, createCFNumber<float>(qpMax / 100.f));
        }
        auto status = VTCompressionSessionCreate(kCFAllocatorDefault, width, height, codecType,
                                                 specs, imageAttrs,
                                                 dataAllocator, &EncodePipeline::output,
                                                 pipeline.get(), &session);
        if (noErr == status) {
            status = VTCompressionSessionPrepareToEncodeFrames(session);
            if (noErr == status) {
                CFBooleanRef hwacclEnabled = nil;
                const auto hwa = noErr == VTSessionCopyProperty(session,
                                                                kVTCompressionPropertyKey_UsingHardwareAcceleratedVideoEncoder,
                                                                nil, &hwacclEnabled) && CFBooleanGetValue(hwacclEnabled);
                return VTEncoderSession(compressedDataAllocator, std::move(pipeline), session, hwa, width, height);
            }
        }
        return COMPLETION_STATUS(status);
    }
    return COMPLETION_STATUS(kVTParameterErr);
}

CFMutableDictionaryRefAutoRelease VTEncoderSession::encoderSpecification(int32_t width, int32_t height)
{
    // Currently hw accl is supported above 360p on mac, below 360p
    // the compression session will be created with hw accl disabled.
    auto specs = CFDictionaryCreateMutable(kCFAllocatorDefault, 1,
                                           &kCFTypeDictionaryKeyCallBacks,
                                           &kCFTypeDictionaryValueCallBacks);
    const bool hwa = std::min(width, height) >= 360;
    CFDictionarySetValue(specs, kVTVideoEncoderSpecification_EnableHardwareAcceleratedVideoEncoder,
                         hwa ? kCFBooleanTrue : kCFBooleanFalse);
    return specs;
}

CFDictionaryRefAutoRelease VTEncoderSession::sourceImageAttributes(int32_t width, int32_t height)
{
    if (width > 0 && height > 0) {
        const auto pixelFormats = createPixelFormats();
        return createImageBufferAttributes(pixelFormats, width, height);
    }
    return nullptr;
}

CFArrayRefAutoRelease VTEncoderSession::createPixelFormats()
{
    const auto formats = CFArrayCreateMutable(kCFAllocatorDefault, 3, &kCFTypeArrayCallBacks);
    for (const auto format : { formatNV12Full(), formatRGB24(), formatBGRA32() }) {
        const auto formatNumber = createCFNumber(static_cast<int64_t>(format), kCFNumberLongType);
        CFArrayAppendValue(formats, formatNumber);
    }
    return formats;
}

VTEncoderSession::EncodePipeline::EncodePipeline(CMVideoCodecType codecType,
                                                 VTEncoderSessionCallback* callback)
    : VTSessionPipeline<VTEncoderSessionCallback>(codecType, callback)
{
}

CompletionStatus VTEncoderSession::EncodePipeline::input(VTCompressionSessionRef session,
                                                         VTEncoderSourceFrame sourceFrame,
                                                         bool forceKeyFrame, VTEncodeInfoFlags* infoFlagsOut)
{
    if (session && sourceFrame) {
        CMTime presentationTimeStamp = CMTimeMake(sourceFrame.timestampUs() / rtc::kNumMicrosecsPerMillisec, 1000);
        CFDictionaryRefAutoRelease frameProperties;
        if (forceKeyFrame) {
            CFTypeRef keys[] = {kVTEncodeFrameOptionKey_ForceKeyFrame};
            CFTypeRef values[] = {kCFBooleanTrue};
            frameProperties.set(CreateCFTypeDictionary(keys, values, 1), false);
        }
        const auto& buffer = sourceFrame.mappedBuffer();
        sourceFrame.setStartTimestamp();
        beginInput();
        return COMPLETION_STATUS(endInput(VTCompressionSessionEncodeFrame(session, buffer,
                                                                          presentationTimeStamp,
                                                                          kCMTimeInvalid,
                                                                          frameProperties,
                                                                          // will be destroyed in [output]
                                                                          new VTEncoderSourceFrame(std::move(sourceFrame)),
                                                                          infoFlagsOut)));
    }
    return COMPLETION_STATUS(kVTParameterErr);
}

void VTEncoderSession::EncodePipeline::output(void* pipeline, void* params,
                                              OSStatus status, VTEncodeInfoFlags infoFlags,
                                              CMSampleBufferRef sampleBuffer)
{
    std::unique_ptr<VTEncoderSourceFrame> sourceFrame(reinterpret_cast<VTEncoderSourceFrame*>(params));
    if (auto selfRef = reinterpret_cast<EncodePipeline*>(pipeline)) {
        const auto statusChanged = selfRef->endOutput(status);
        if (noErr == status) {
            sourceFrame->setFinishTimestamp();
            selfRef->callback(&VTEncoderSessionCallback::onEncodedImage,
                              VTEncoderSourceFrame(std::move(*sourceFrame)),
                              infoFlags, sampleBuffer);
        }
        else if (statusChanged) {
            selfRef->callback(&VTEncoderSessionCallback::onError, COMPLETION_STATUS(status), true);
        }
    }
}

} // namespace LiveKitCpp
