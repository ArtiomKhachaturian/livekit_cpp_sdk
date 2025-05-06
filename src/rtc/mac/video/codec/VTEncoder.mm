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
#include "VTEncoder.h"
#include "VideoFrameBufferPoolSource.h"
#include "VideoUtils.h"
#include "VTEncoderSourceFrame.h"
#include "VTEncoderSession.h"
#include "CFMemoryPool.h"
#include "Utils.h"
#include <Foundation/Foundation.h>

namespace LiveKitCpp
{

VTEncoder::VTEncoder(const webrtc::SdpVideoFormat& format,
                     webrtc::CodecSpecificInfo codecSpecificInfo,
                     const std::shared_ptr<CFMemoryPool>& memoryPool)
    : VideoEncoder(format, std::move(codecSpecificInfo), true)
    , _memoryPool(memoryPool ? memoryPool : CFMemoryPool::create())
    , _framesPool(VideoFrameBufferPoolSource::create())
{
}

VTEncoder::~VTEncoder()
{
    VTEncoder::destroySession();
}

bool VTEncoder::hardwareAccelerated() const
{
    if (_session) {
        return _session.hardwareAccelerated();
    }
    return VideoEncoder::hardwareAccelerated();
}

int32_t VTEncoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    int32_t r = VideoEncoder::InitEncode(codecSettings, encoderSettings);
    if (WEBRTC_VIDEO_CODEC_OK == r) {
        const auto status = logError(createSession(codecSettings->width, codecSettings->height));
        if (!status) {
            r = WEBRTC_VIDEO_CODEC_ERROR;
        }
    }
    return r;
}

int32_t VTEncoder::Encode(const webrtc::VideoFrame& frame,
                          const std::vector<webrtc::VideoFrameType>* frameTypes)
{
    if (!hasEncodeCompleteCallback() || !_session) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (dropNextFrame()) {
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    auto frameResult = createSourceFrame(frame);
    if (!frameResult) {
        logError(frameResult.moveStatus());
        return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    bool keyFrameRequired = false, sessionIsOutdate = false;
    auto sourceFrame = frameResult.moveValue();
    auto status = _session.lastOutputStatus();
    switch (status.code()) {
        case kVTVideoEncoderMalfunctionErr:
        case kVTInvalidSessionErr:
            sessionIsOutdate = true;
            logError(std::move(status), false);
            break;
        case noErr:
            sessionIsOutdate = !_session.isCompatible(sourceFrame)
                || _session.pendingFramesCount() >= maxFramerate();
            break;
        default:
            logError(std::move(status));
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    if (sessionIsOutdate) {
        // re-create compression session
        auto sessionResult = createSession(sourceFrame.width(), sourceFrame.height());
        if (sessionResult) {
            keyFrameRequired = true;
        }
        else {
            logError(std::move(sessionResult));
            return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
        }
    }
    if (!keyFrameRequired) {
        keyFrameRequired = keyFrameRequested(frameTypes);
    }
    status = _session.compress(std::move(sourceFrame), keyFrameRequired);
    if (kVTInvalidSessionErr == status.code() || kVTVideoEncoderMalfunctionErr == status.code()) {
        // this error occurs when entering foreground after backgrounding the app
        // sometimes the encoder malfunctions and needs to be restarted
        logError(std::move(status), false);
        // re-create compression session
        auto sessionResult = createSession(_session.width(), _session.height());
        if (sessionResult) {
            return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
        }
        logError(std::move(sessionResult));
        return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    if (!status) {
        logError(std::move(status));
        return WEBRTC_VIDEO_CODEC_ENCODER_FAILURE;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

webrtc::VideoEncoder::EncoderInfo VTEncoder::GetEncoderInfo() const
{
    auto encoderInfo = VideoEncoder::GetEncoderInfo();
    addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type::kNV12, encoderInfo);
    addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type::kI420, encoderInfo);
    return encoderInfo;
}

CompletionStatus VTEncoder::configureCompressionSession(VTEncoderSession* session)
{
    if (session) {
        auto status = session->prepareToEncodeFrames();
        if (status) {
            status = session->setRealTime(true);
            if (status) {
                status = session->setAllowFrameReordering(false);
                if (status && maxBitrateBps() / 8U > 0U) {
                    // that say we set data in byte/second
                    status = session->setDataRateLimits(maxBitrateBps());
                }
            }
        }
        return status;
    }
    return COMPLETION_STATUS(kVTParameterErr);
}

void VTEncoder::destroySession()
{
    if (_session) {
        logError(_session.completeFrames(), false);
        _session = {};
    }
    if (_framesPool) {
        _framesPool->release();
    }
    VideoEncoder::destroySession();
}

CompletionStatus VTEncoder::setEncoderBitrate(uint32_t bitrateBps)
{
    if (_session) {
        return _session.setAverageBitRate(bitrateBps);
    }
    return {};
}

CompletionStatus VTEncoder::setEncoderFrameRate(uint32_t frameRate)
{
    if (_session) {
        return _session.setExpectedFrameRate(frameRate);
    }
    return {};
}

CompletionStatusOr<VTEncoderSourceFrame> VTEncoder::createSourceFrame(const webrtc::VideoFrame& frame) const
{
    return VTEncoderSourceFrame::create(frame, VideoFrameBufferPool{_framesPool});
}

CompletionStatus VTEncoder::createSession(int32_t width, int32_t height)
{
    destroySession();
    if (const auto vtType = toVTCodecType(type())) {
        auto session = VTEncoderSession::create(width, height, vtType.value(),
                                                qpMax(), this, _memoryPool);
        if (session) {
            auto status = configureCompressionSession(&session.value());
            if (status.ok()) {
                _session = session.moveValue();
                status = setEncoderBitrate(currentBitrate());
                if (status) {
                    status = setEncoderFrameRate(currentFramerate());
                }
                if (!status) {
                    _session = {};
                }
            }
            return status;
        }
        return session.moveStatus();
    }
    return COMPLETION_STATUS(kVTCouldNotFindVideoEncoderErr);
}

void VTEncoder::onEncodedImage(VTEncoderSourceFrame frame,
                               VTEncodeInfoFlags infoFlags,
                               CMSampleBufferRef sampleBuffer)
{
    if (hasEncodeCompleteCallback()) {
        if (testFlag<kVTEncodeInfo_FrameDropped>(infoFlags) || !CMSampleBufferDataIsReady(sampleBuffer)) {
            dropEncodedImage(webrtc::EncodedImageCallback::DropReason::kDroppedByEncoder);
        }
        else {
            bool keyFrame = false;
            CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, 0);
            if (attachments && CFArrayGetCount(attachments)) {
                auto attachment = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(attachments, 0));
                keyFrame = !CFDictionaryContainsKey(attachment, kCMSampleAttachmentKey_NotSync);
            }
            auto result = createEncodedImageFromSampleBuffer(sampleBuffer, keyFrame, _memoryPool.get());
            if (result) {
                webrtc::EncodedImage encodedImage;
                encodedImage.capture_time_ms_ = frame.renderTimeMs();
                encodedImage.SetEncodedData(result.moveValue());
                encodedImage._encodedWidth = frame.width();
                encodedImage._encodedHeight = frame.height();
                encodedImage.qp_ = lastQp();
                encodedImage.rotation_ = frame.rotation();
                encodedImage.SetRtpTimestamp(frame.timestampRtp());
                encodedImage.SetEncodeTime(frame.startTimestampMs(), frame.finishTimestampMs());
                sendEncodedImage(keyFrame, std::move(encodedImage));
            }
            else {
                logError(result.moveStatus(), false);
            }
        }
    }
}

void VTEncoder::onError(CompletionStatus error, bool fatal)
{
    logError(std::move(error), fatal);
}

} // namespace LiveKitCpp
