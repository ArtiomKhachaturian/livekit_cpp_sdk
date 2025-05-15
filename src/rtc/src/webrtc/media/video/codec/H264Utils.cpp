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
#include "H264Utils.h"
#include "VideoUtils.h"
#include "Utils.h"
#include <media/base/media_constants.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <cassert>
#include <optional>

namespace
{

using namespace LiveKitCpp;

float maxAllowedFrameRateH264(webrtc::H264Level level, uint16_t width, uint16_t height, float maxFrameRate);

template <class TCodecSource>
inline float maxAllowedFrameRate(webrtc::H264Level level, const TCodecSource* source)
{
    if (source) {
        return maxAllowedFrameRateH264(level, static_cast<uint16_t>(source->width),
                                       static_cast<uint16_t>(source->height),
                                       source->maxFramerate);
    }
    return 0.f;
}

#ifdef WEBRTC_WIN
inline std::vector<webrtc::SdpVideoFormat> mftH264Codecs() {
    std::vector<webrtc::SdpVideoFormat> codecs;
    codecs.reserve(4U);
    codecs.push_back(webrtc::CreateH264Format(webrtc::H264Profile::kProfileBaseline, webrtc::H264Level::kLevel3_1, "1"));
    codecs.push_back(webrtc::CreateH264Format(webrtc::H264Profile::kProfileBaseline, webrtc::H264Level::kLevel3_1, "0"));
    codecs.push_back(webrtc::CreateH264Format(webrtc::H264Profile::kProfileConstrainedBaseline, webrtc::H264Level::kLevel3_1, "1"));
    codecs.push_back(webrtc::CreateH264Format(webrtc::H264Profile::kProfileConstrainedBaseline, webrtc::H264Level::kLevel3_1, "0"));
    return codecs;
}
#endif

#ifdef USE_PLATFORM_ENCODERS
#ifdef WEBRTC_WIN
inline std::vector<webrtc::SdpVideoFormat> h264Encoders() {
    return mftH264Codecs();
}
#else
inline std::vector<webrtc::SdpVideoFormat> h264Encoders() {
    return webrtc::SupportedH264Codecs(true);
}
#endif
#endif

#ifdef USE_PLATFORM_DECODERS
#ifdef WEBRTC_WIN
inline std::vector<webrtc::SdpVideoFormat> h264Decoders() {
    return mftH264Codecs();
}
#else
inline std::vector<webrtc::SdpVideoFormat> h264Decoders() {
    return webrtc::SupportedH264DecoderCodecs();
}
#endif
#endif

} // namespace

namespace LiveKitCpp
{

std::vector<webrtc::SdpVideoFormat> H264Utils::platformEncoderFormats()
{
#ifdef USE_PLATFORM_ENCODERS
    auto formats = h264Encoders();
    auto it = std::remove_if(formats.begin(), formats.end(), [](const webrtc::SdpVideoFormat& format) {
        return CodecStatus::NotSupported == encoderStatus(format);
    });
    formats.erase(it, formats.end());
    return formats;
#else
    return {};
#endif
}

std::vector<webrtc::SdpVideoFormat> H264Utils::platformDecoderFormats()
{
#ifdef USE_PLATFORM_DECODERS
    auto formats = h264Decoders();
    auto it = std::remove_if(formats.begin(), formats.end(), [](const webrtc::SdpVideoFormat& format) {
        return CodecStatus::NotSupported == decoderStatus(format);
    });
    formats.erase(it, formats.end());
    return formats;
#else
    return {};
#endif
}

bool H264Utils::formatMatched(const webrtc::SdpVideoFormat& format)
{
    return codecNameMatched(format.name);
}

bool H264Utils::codecNameMatched(const std::string& codecName)
{
    return webrtc::VideoCodecType::kVideoCodecH264 == webrtc::PayloadStringToCodecType(codecName);
}

webrtc::VideoCodec H264Utils::map(const webrtc::VideoCodec& settings, webrtc::H264Level level)
{
    webrtc::VideoCodec h264;
    h264.codecType = webrtc::VideoCodecType::kVideoCodecH264;
    h264.width = settings.width;
    h264.height = settings.height;
    h264.startBitrate = settings.startBitrate;
    h264.maxBitrate = settings.maxBitrate;
    h264.minBitrate = settings.minBitrate;
    h264.maxFramerate = static_cast<uint32_t>(maxAllowedFrameRate(level, &settings) + 0.5f);
    h264.active = settings.active;
    h264.qpMax = settings.qpMax;
    h264.numberOfSimulcastStreams = settings.numberOfSimulcastStreams;
    for (int i = 0; i < webrtc::kMaxSimulcastStreams; ++i) {
        h264.simulcastStream[i] = settings.simulcastStream[i];
        h264.simulcastStream[i].maxFramerate = maxAllowedFrameRate(level, &h264.simulcastStream[i]);
    }
    for (int i = 0; i < webrtc::kMaxSpatialLayers; ++i) {
        h264.spatialLayers[i] = settings.spatialLayers[i];
        h264.spatialLayers[i].maxFramerate = maxAllowedFrameRate(level, &h264.spatialLayers[i]);
    }
    h264.mode = settings.mode;
    h264.expect_encode_from_texture = settings.expect_encode_from_texture;
    h264.timing_frame_thresholds = settings.timing_frame_thresholds;
    h264.legacy_conference_mode = settings.legacy_conference_mode;
    //h264.H264()->frameDroppingOn = settings.H264().frameDroppingOn;
    h264.H264()->keyFrameInterval = settings.H264().keyFrameInterval;
    h264.H264()->numberOfTemporalLayers = settings.H264().numberOfTemporalLayers;
    return h264;
}

webrtc::H264PacketizationMode H264Utils::packetizationMode(const webrtc::SdpVideoFormat& format)
{
    const auto it = format.parameters.find(cricket::kH264FmtpPacketizationMode);
    if (it != format.parameters.end()) {
        return packetizationMode(it->second);
    }
    return webrtc::H264PacketizationMode::SingleNalUnit;
}

webrtc::H264PacketizationMode H264Utils::packetizationMode(const std::string_view& mode)
{
    if (compareCaseInsensitive(mode, "1")) {
        return webrtc::H264PacketizationMode::NonInterleaved;
    }
    return webrtc::H264PacketizationMode::SingleNalUnit;
}

webrtc::CodecSpecificInfo H264Utils::createCodecInfo(const webrtc::SdpVideoFormat& format)
{
    return createCodecInfo(packetizationMode(format));
}

webrtc::CodecSpecificInfo H264Utils::createCodecInfo(webrtc::H264PacketizationMode packetizationMode)
{
    webrtc::CodecSpecificInfo codecInfo;
    codecInfo.codecType = webrtc::VideoCodecType::kVideoCodecH264;
    codecInfo.codecSpecific.H264.packetization_mode = packetizationMode;
    return codecInfo;
}

bool H264Utils::cabacIsSupported(webrtc::H264Profile profile)
{
    return profile >= webrtc::H264Profile::kProfileMain;
}

} // namespace LiveKitCpp


namespace
{
    
inline unsigned long maxSampleRateH264(webrtc::H264Level level)
{
    switch (level) {
        case webrtc::H264Level::kLevel3:
            return 10368000;
        case webrtc::H264Level::kLevel3_1:
            return 27648000;
        case webrtc::H264Level::kLevel3_2:
            return 55296000;
        case webrtc::H264Level::kLevel4:
        case webrtc::H264Level::kLevel4_1:
            return 62914560;
        case webrtc::H264Level::kLevel4_2:
            return 133693440;
        case webrtc::H264Level::kLevel5:
            return 150994944;
        case webrtc::H264Level::kLevel5_1:
            return 251658240;
        case webrtc::H264Level::kLevel5_2:
            return 530841600;
        case webrtc::H264Level::kLevel1:
        case webrtc::H264Level::kLevel1_b:
        case webrtc::H264Level::kLevel1_1:
        case webrtc::H264Level::kLevel1_2:
        case webrtc::H264Level::kLevel1_3:
        case webrtc::H264Level::kLevel2:
        case webrtc::H264Level::kLevel2_1:
        case webrtc::H264Level::kLevel2_2:
        default:
            // Zero means auto rate setting.
            break;
    }
    return 0; // zero means auto rate setting
}
    
float maxAllowedFrameRateH264(webrtc::H264Level level, uint16_t width,
                              uint16_t height, float maxFrameRate)
{
    if (width && height) {
        if (const auto maxSampleRate = maxSampleRateH264(level)) {
            const uint32_t alignedWidth = (((width + 15) >> 4) << 4);
            const uint32_t alignedHeight = (((height + 15) >> 4) << 4);
            return std::min<float>(maxFrameRate, (1.f * maxSampleRate) / (alignedWidth * alignedHeight));
        }
        return maxFrameRate;
    }
    return 0.f;
}
    
}
