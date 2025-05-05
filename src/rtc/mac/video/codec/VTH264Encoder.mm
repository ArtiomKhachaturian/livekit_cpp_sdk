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
#include "VTH264Encoder.h"
#include "VTH264EncodedBuffer.h"
#include "VTEncoderSession.h"
#include "VideoUtils.h"
#include "H264Utils.h"
#include "EncodedImageBuffer.h"
#include <components/video_codec/nalu_rewriter.h>


namespace LiveKitCpp
{

VTH264Encoder::VTH264Encoder(const webrtc::SdpVideoFormat& format,
                             webrtc::H264PacketizationMode mode)
    : VTEncoder(format, H264Utils::createCodecInfo(mode))
    , _profileLevelId(webrtc::ParseSdpForH264ProfileLevelId(format.parameters))
{
}

VTH264Encoder::~VTH264Encoder()
{
}

std::unique_ptr<webrtc::VideoEncoder> VTH264Encoder::create(const webrtc::SdpVideoFormat& format)
{
    std::unique_ptr<webrtc::VideoEncoder> encoder;
    if (H264Utils::formatMatched(format)) {
        const auto status = encoderStatus(format);
        if (CodecStatus::NotSupported != status) {
            const auto packetizationMode = H264Utils::packetizationMode(format);
            encoder.reset(new VTH264Encoder(format, packetizationMode));
        }
    }
    return encoder;
}

CFStringRef VTH264Encoder::extractProfile(const webrtc::H264ProfileLevelId& profileLevelId)
{
    switch (profileLevelId.profile) {
        case webrtc::H264Profile::kProfileConstrainedBaseline:
        case webrtc::H264Profile::kProfileBaseline:
            switch (profileLevelId.level) {
                case webrtc::H264Level::kLevel3:
                    return kVTProfileLevel_H264_Baseline_3_0;
                case webrtc::H264Level::kLevel3_1:
                    return kVTProfileLevel_H264_Baseline_3_1;
                case webrtc::H264Level::kLevel3_2:
                    return kVTProfileLevel_H264_Baseline_3_2;
                case webrtc::H264Level::kLevel4:
                    return kVTProfileLevel_H264_Baseline_4_0;
                case webrtc::H264Level::kLevel4_1:
                    return kVTProfileLevel_H264_Baseline_4_1;
                case webrtc::H264Level::kLevel4_2:
                    return kVTProfileLevel_H264_Baseline_4_2;
                case webrtc::H264Level::kLevel5:
                    return kVTProfileLevel_H264_Baseline_5_0;
                case webrtc::H264Level::kLevel5_1:
                    return kVTProfileLevel_H264_Baseline_5_1;
                case webrtc::H264Level::kLevel5_2:
                    return kVTProfileLevel_H264_Baseline_5_2;
                case webrtc::H264Level::kLevel1:
                case webrtc::H264Level::kLevel1_b:
                case webrtc::H264Level::kLevel1_1:
                case webrtc::H264Level::kLevel1_2:
                case webrtc::H264Level::kLevel1_3:
                case webrtc::H264Level::kLevel2:
                case webrtc::H264Level::kLevel2_1:
                case webrtc::H264Level::kLevel2_2:
                    return kVTProfileLevel_H264_Baseline_AutoLevel;
                default:
                    break;
            }
            break;
        case webrtc::H264Profile::kProfileMain:
            switch (profileLevelId.level) {
                case webrtc::H264Level::kLevel3:
                    return kVTProfileLevel_H264_Main_3_0;
                case webrtc::H264Level::kLevel3_1:
                    return kVTProfileLevel_H264_Main_3_1;
                case webrtc::H264Level::kLevel3_2:
                    return kVTProfileLevel_H264_Main_3_2;
                case webrtc::H264Level::kLevel4:
                    return kVTProfileLevel_H264_Main_4_0;
                case webrtc::H264Level::kLevel4_1:
                    return kVTProfileLevel_H264_Main_4_1;
                case webrtc::H264Level::kLevel4_2:
                    return kVTProfileLevel_H264_Main_4_2;
                case webrtc::H264Level::kLevel5:
                    return kVTProfileLevel_H264_Main_5_0;
                case webrtc::H264Level::kLevel5_1:
                    return kVTProfileLevel_H264_Main_5_1;
                case webrtc::H264Level::kLevel5_2:
                    return kVTProfileLevel_H264_Main_5_2;
                case webrtc::H264Level::kLevel1:
                case webrtc::H264Level::kLevel1_b:
                case webrtc::H264Level::kLevel1_1:
                case webrtc::H264Level::kLevel1_2:
                case webrtc::H264Level::kLevel1_3:
                case webrtc::H264Level::kLevel2:
                case webrtc::H264Level::kLevel2_1:
                case webrtc::H264Level::kLevel2_2:
                    return kVTProfileLevel_H264_Main_AutoLevel;
                default:
                    break;
            }
            break;
        case webrtc::H264Profile::kProfileConstrainedHigh:
        case webrtc::H264Profile::kProfileHigh:
        //case webrtc::H264Profile::kProfilePredictiveHigh444:
            switch (profileLevelId.level) {
                case webrtc::H264Level::kLevel3:
                    return kVTProfileLevel_H264_High_3_0;
                case webrtc::H264Level::kLevel3_1:
                    return kVTProfileLevel_H264_High_3_1;
                case webrtc::H264Level::kLevel3_2:
                    return kVTProfileLevel_H264_High_3_2;
                case webrtc::H264Level::kLevel4:
                    return kVTProfileLevel_H264_High_4_0;
                case webrtc::H264Level::kLevel4_1:
                    return kVTProfileLevel_H264_High_4_1;
                case webrtc::H264Level::kLevel4_2:
                    return kVTProfileLevel_H264_High_4_2;
                case webrtc::H264Level::kLevel5:
                    return kVTProfileLevel_H264_High_5_0;
                case webrtc::H264Level::kLevel5_1:
                    return kVTProfileLevel_H264_High_5_1;
                case webrtc::H264Level::kLevel5_2:
                    return kVTProfileLevel_H264_High_5_2;
                case webrtc::H264Level::kLevel1:
                case webrtc::H264Level::kLevel1_b:
                case webrtc::H264Level::kLevel1_1:
                case webrtc::H264Level::kLevel1_2:
                case webrtc::H264Level::kLevel1_3:
                case webrtc::H264Level::kLevel2:
                case webrtc::H264Level::kLevel2_1:
                case webrtc::H264Level::kLevel2_2:
                    return kVTProfileLevel_H264_High_AutoLevel;
                default:
                    break;
            }
        default:
            break;
    }
    return nullptr;
}

int32_t VTH264Encoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    int32_t result = WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    if (codecSettings) {
        _outputBufferCacheSize = H264BitstreamParser::annexBHeaderSize();
        _keyFrameInterval = codecSettings->H264().keyFrameInterval;
        if (_profileLevelId) {
            const auto h264 = H264Utils::map(*codecSettings, _profileLevelId->level);
            result = VTEncoder::InitEncode(&h264, encoderSettings);
        }
        else {
            result = VTEncoder::InitEncode(codecSettings, encoderSettings);
        }
    }
    return result;
}

int32_t VTH264Encoder::Release()
{
    _outputBufferCacheSize = H264BitstreamParser::annexBHeaderSize();
    return VTEncoder::Release();
}

webrtc::VideoEncoder::EncoderInfo VTH264Encoder::GetEncoderInfo() const
{
    auto encoderInfo = VTEncoder::GetEncoderInfo();
    encoderInfo.is_qp_trusted = true;
    return encoderInfo;
}

void VTH264Encoder::destroySession()
{
    VTEncoder::destroySession();
    _h264BitstreamParser.reset();
}

webrtc::RTCError VTH264Encoder::configureCompressionSession(VTEncoderSession* session)
{
    auto status = VTEncoder::configureCompressionSession(session);
    if (status.ok()) {
        if (_keyFrameInterval > 0) { // https://bugs.chromium.org/p/webrtc/issues/detail?id=5815
            session->setMaxKeyFrameInterval(_keyFrameInterval);
        }
        if (_profileLevelId) {
            session->setProfileLevel(extractProfile(_profileLevelId.value()));
            // or maybe choose CABAC explicitly, without profile checking?
            if (H264Utils::cabacIsSupported(_profileLevelId->profile)) {
                // CABAC generally gives better compression at the expense of higher computational overhead,
                // for avoding of bitrate overshoot
                session->setProperty(kVTCompressionPropertyKey_H264EntropyMode, kVTH264EntropyMode_CABAC);
            }
        }
    }
    return status;
}

RtcErrorOrEncodedImageBuffer VTH264Encoder::createEncodedImageFromSampleBuffer(CMSampleBufferRef sampleBuffer,
                                                                               bool isKeyFrame,
                                                                               const CFMemoryPool* memoryPool)
{
    if (sampleBuffer) {
        webrtc::scoped_refptr<webrtc::EncodedImageBufferInterface> encodedBuffer;
        if (memoryPool) {
            auto result = VTH264EncodedBuffer::create(memoryPool, sampleBuffer, isKeyFrame, _outputBufferCacheSize);
            if (!result.ok()) {
                return result.MoveError();
            }
            encodedBuffer = result.MoveValue();
        }
        // non-cached version
        if (!encodedBuffer) {
            rtc::Buffer buffer;
            if (!webrtc::H264CMSampleBufferToAnnexBBuffer(sampleBuffer, isKeyFrame, &buffer)) {
                return toRtcError(kCMSampleBufferError_InvalidMediaTypeForOperation);
            }
            encodedBuffer = EncodedImageBuffer::create(std::move(buffer));
        }
        if (encodedBuffer) {
            _outputBufferCacheSize = std::max(_outputBufferCacheSize, encodedBuffer->size());
            _h264BitstreamParser.parseForSliceQp(encodedBuffer);
            return encodedBuffer;
        }
    }
    return toRtcError(kVTParameterErr, webrtc::RTCErrorType::INVALID_PARAMETER);
}

} // namespace LiveKitCpp
