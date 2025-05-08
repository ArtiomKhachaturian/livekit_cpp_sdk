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
#include "CFMemoryPool.h"
#include "RtcUtils.h"
#include "VideoFrameBufferPoolSource.h"
#include "Utils.h"

namespace LiveKitCpp
{

VTDecoder::VTDecoder(const webrtc::SdpVideoFormat& format,
                     const std::shared_ptr<CFMemoryPool>& memoryPool,
                     OSType outputPixelFormat)
    : VideoDecoder(format)
    , _outputPixelFormat(outputPixelFormat)
    , _memoryPool(memoryPool ? memoryPool : CFMemoryPool::create())
    , _framesPool(VideoFrameBufferPoolSource::create())
{
}

VTDecoder::~VTDecoder()
{
    VTDecoder::destroySession();
}

bool VTDecoder::hardwareAccelerated() const
{
    if (_session) {
        return _session.hardwareAccelerated();
    }
    return VideoDecoder::hardwareAccelerated();
}

bool VTDecoder::Configure(const Settings& settings)
{
    if (VideoDecoder::Configure(settings)) {
        _numberOfCores = std::max(0, settings.number_of_cores());
        if (const auto imageFormat = createInitialVideoFormat(settings.max_render_resolution())) {
            return logError(createSession(imageFormat, true)).ok();
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
        if (!status) {
            logError(std::move(status));
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
    }
    const auto encodeBuffer = inputImage.GetEncodedData();
    if (!encodeBuffer) {
        logWarning(COMPLETION_STATUS(kVTVideoDecoderUnsupportedDataFormatErr));
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    if (webrtc::VideoFrameType::kVideoFrameKey == inputImage._frameType) {
        const auto imageFormat = createVideoFormat(inputImage);
        if (!imageFormat) {
            logError(COMPLETION_STATUS(kVTVideoDecoderUnsupportedDataFormatErr));
            return WEBRTC_VIDEO_CODEC_ERROR;
        }
        // check if the video format has changed, and reinitialize decoder if needed
        if (!_session || !CMFormatDescriptionEqual(imageFormat, _session.format())) {
            const bool realtime = webrtc::VideoContentType::UNSPECIFIED == inputImage.content_type_;
            const auto sessionStatus = logError(createSession(imageFormat, realtime)); // retain
            if (!sessionStatus) {
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
        }
        else {
            CFRelease(imageFormat);
        }
    }
    const auto sampleBuffer = createSampleBuffer(inputImage, _session.format());
    if (!sampleBuffer) {
        logWarning(COMPLETION_STATUS(kCMBlockBufferEmptyBBufErr));
        return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
    }
    VTDecodeInfoFlags infoFlags;
    auto status = _session.decompress(sampleBuffer, inputImage, &infoFlags);
    if (!status) {
        logError(std::move(status));
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
        logWarning(_session.waitForAsynchronousFrames());
        _session = {};
    }
    VideoDecoder::destroySession();
}

CompletionStatus VTDecoder::createSession(CFAutoRelease<CMVideoFormatDescriptionRef> format, bool realtime)
{
    destroySession();
    if (const auto vtCodec = toVTCodecType(type())) {
        auto session = VTDecoderSession::create(vtCodec.value(),
                                                std::move(format),
                                                realtime,
                                                _outputPixelFormat,
                                                _numberOfCores,
                                                this,
                                                VideoFrameBufferPool{_framesPool});
        if (session) {
            _session = session.moveValue();
            if (_session) {
                const auto poolSize = bufferPoolSize();
                if (poolSize > 0) {
                    if (_framesPool) {
                        _framesPool->resize(poolSize);
                    }
                    logWarning(_session.setOutputPoolRequestedMinimumBufferCount(poolSize));
                }
            }
        }
        return session.moveStatus();
    }
    return COMPLETION_STATUS(kVTCouldNotFindVideoDecoderErr);
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
            logWarning(COMPLETION_STATUS(kVTVideoDecoderMalfunctionErr));
        }
    }
}

void VTDecoder::onError(CompletionStatus error, bool fatal)
{
    if (fatal) {
        logError(std::move(error));
    }
    else {
        logWarning(std::move(error));
    }
}

} // namespace LiveKitCpp
