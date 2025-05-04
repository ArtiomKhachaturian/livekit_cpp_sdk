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
#include "VTDecoder.h"
#include "VideoFrameBufferPoolSource.h"
#include "VideoUtils.h"
#include "Utils.h"

namespace LiveKitCpp
{

VTDecoder::VTDecoder(OSType outputPixelFormat,
                     webrtc::VideoCodecType codecType,
                     bool hardwareAccelerated,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     const std::shared_ptr<CFMemoryPool>& memoryPool)
    : VideoDecoder(codecType, hardwareAccelerated, logger)
    , _outputPixelFormat(outputPixelFormat)
    , _memoryPool(memoryPool)
    , _framesPool(VideoFrameBufferPoolSource::create())
{
}

VTDecoder::~VTDecoder()
{
    VTDecoder::destroySession();
}

CodecStatus VTDecoder::hardwaredDecodeSupported(CMVideoCodecType codecType)
{
    if (@available(macOS 10.9, *)) {
        static std::once_flag registerProfessionalVideoWorkflowVideoDecoders;
        std::call_once(registerProfessionalVideoWorkflowVideoDecoders, VTRegisterProfessionalVideoWorkflowVideoDecoders);
    }
    if (@available(macOS 11.0, *)) {
        VTRegisterSupplementalVideoDecoderIfAvailable(codecType);
    }
    if (VTIsHardwareDecodeSupported(codecType)) {
        return CodecStatus::SupportedHardware;
    }
    return CodecStatus::SupportedSoftware;
}

bool VTDecoder::Configure(const Settings& settings)
{
    if (VideoDecoder::Configure(settings)) {
        _numberOfCores = std::max(0, settings.number_of_cores());
        if (const auto imageFormat = createInitialVideoFormat(settings.max_render_resolution())) {
            return log(createSession(imageFormat, true)).ok();
        }
        return true; // no wait of acceptable video format, maybe in VTDecoder::Decode
    }
    return false;
}

int32_t VTDecoder::Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs)
{
    if (!hasDecodeCompleteCallback()) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (_session) {
        auto status = _session.lastOutputStatus();
        if (noErr != status) {
            log(toRtcError(status));
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
    }
    const auto encodeBuffer = inputImage.GetEncodedData();
    if (!encodeBuffer) {
        log(toRtcError(kVTVideoDecoderUnsupportedDataFormatErr), false);
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    if (webrtc::VideoFrameType::kVideoFrameKey == inputImage._frameType) {
        const auto imageFormat = createVideoFormat(inputImage);
        if (!imageFormat) {
            log(toRtcError(kVTVideoDecoderUnsupportedDataFormatErr));
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        // check if the video format has changed, and reinitialize decoder if needed
        if (!_session || !CMFormatDescriptionEqual(imageFormat, _session.format())) {
            const bool realtime = webrtc::VideoContentType::UNSPECIFIED == inputImage.content_type_;
            const auto sessionStatus = log(createSession(imageFormat, realtime)); // retain
            if (!sessionStatus.ok()) {
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
        }
        else {
            CFRelease(imageFormat);
        }
    }
    const auto sampleBuffer = createSampleBuffer(inputImage, _session.format());
    if (!sampleBuffer) {
        log(toRtcError(kCMBlockBufferEmptyBBufErr), false);
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    VTDecodeInfoFlags infoFlags;
    auto decompressStatus = _session.decompress(sampleBuffer, inputImage, &infoFlags);
    if (!decompressStatus.ok()) {
        log(std::move(decompressStatus));
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    if (testFlag<kVTDecodeInfo_FrameDropped>(infoFlags)) {
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    return WEBRTC_VIDEO_CODEC_OK;
}

CMVideoFormatDescriptionRef VTDecoder::createInitialVideoFormat(const webrtc::RenderResolution& resolution) const
{
    if (resolution.Valid()) {
        return createInitialVideoFormat(resolution.Width(), resolution.Height());
    }
    return nullptr;
}

CMVideoFormatDescriptionRef VTDecoder::createInitialVideoFormat(uint32_t encodedWidth,
                                                                uint32_t encodedHeight) const
{
    return nullptr;
}

void VTDecoder::destroySession()
{
    if (_session) {
        log(_session.waitForAsynchronousFrames(), false);
        _session = {};
    }
    VideoDecoder::destroySession();
}

webrtc::RTCError VTDecoder::createSession(CFAutoRelease<CMVideoFormatDescriptionRef> format, bool realtime)
{
    destroySession();
    if (const auto vtCodec = toVTCodecType(type())) {
        auto session = VTDecoderSession::create(hardwareAccelerated(),
                                                vtCodec.value(),
                                                std::move(format),
                                                _outputPixelFormat,
                                                _numberOfCores,
                                                this,
                                                VideoFrameBufferPool{_framesPool},
                                                logger(),
                                                realtime);
        if (session.ok()) {
            _session = session.MoveValue();
            if (_session) {
                const auto poolSize = bufferPoolSize();
                if (poolSize > 0) {
                    if (_framesPool) {
                        _framesPool->resize(poolSize);
                        log(_session.setOutputPoolRequestedMinimumBufferCount(poolSize), false);
                    }
                }
            }
        }
        return session.error();
    }
    return toRtcError(kVTCouldNotFindVideoDecoderErr);
}

void VTDecoder::onDecodedImage(CMTime timestamp, CMTime duration,
                               VTDecodeInfoFlags infoFlags,
                               uint32_t encodedImageTimestamp,
                               webrtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer,
                               std::optional<uint8_t> qp,
                               std::optional<webrtc::ColorSpace> encodedImageColorspace)
{
    if (hasDecodeCompleteCallback() && !testFlag<kVTDecodeInfo_FrameDropped>(infoFlags) && buffer) {
        if (auto frame = createVideoFrame(buffer,
                                          webrtc::VideoRotation::kVideoRotation_0,
                                          cmTimeToMicro(timestamp),
                                          0U,
                                          encodedImageColorspace)) {
            frame->set_rtp_timestamp(encodedImageTimestamp);
            sendDecodedImage(frame.value(), cmTimeToMilli(duration), std::move(qp));
        }
        else {
            log(toRtcError(kVTVideoDecoderMalfunctionErr), false);
        }
    }
}

void VTDecoder::onError(OSStatus error, bool fatal)
{
    log(toRtcError(error), fatal);
}

CodecStatus VideoDecoder::checkDecoderSupport(const webrtc::SdpVideoFormat& format)
{
    CodecStatus status = CodecStatus::NotSupported;
    const auto codecType = webrtc::PayloadStringToCodecType(format.name);
    if (webrtc::VideoCodecType::kVideoCodecGeneric != codecType) {
        switch (codecType) {
            case webrtc::kVideoCodecVP8:
            case webrtc::kVideoCodecVP9:
            case webrtc::kVideoCodecAV1:
                return CodecStatus::SupportedSoftware;
            default:
                break;
        }
        if (webrtc::kVideoCodecH264 == codecType) {
            static const auto h264Status = VTDecoder::hardwaredDecodeSupported(codecTypeH264());
            status = h264Status;
        }
    }
    return status;
}


} // namespace LiveKitCpp
