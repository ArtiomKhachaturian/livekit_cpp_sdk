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
#include "VTVideoEncoderFactory.h"
#ifdef USE_PLATFORM_ENCODERS
#include "VTEncoderSession.h"
#include "VTH264Encoder.h"
#include "H264Utils.h"
#include "VideoUtils.h"

namespace
{

using namespace LiveKitCpp;

VTEncoderSession createCompressor(CMVideoCodecType codecType,
                                  int32_t width = 640, int32_t height = 480,
                                  CFStringRef profile = nullptr);

}

namespace LiveKitCpp
{

VTVideoEncoderFactory::VTVideoEncoderFactory()
    : _h264Formats(H264Utils::platformEncoderFormats())
{
}

std::unique_ptr<webrtc::VideoEncoder> VTVideoEncoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    if (auto encoder = VTH264Encoder::create(format)) {
        return encoder;
    }
    return {};
}

webrtc::VideoEncoderFactory::CodecSupport VTVideoEncoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format, std::optional<std::string> scalabilityMode) const
{
    auto support = webrtc::VideoEncoderFactory::QueryCodecSupport(format, scalabilityMode);
    if (support.is_supported) {
        support.is_power_efficient = maybeHardwareAccelerated(encoderStatus(format));
    }
    return support;
}

CodecStatus platformEncoderStatus(webrtc::VideoCodecType type, const webrtc::CodecParameterMap& parameters)
{
    CodecStatus status = CodecStatus::NotSupported;
    if (webrtc::VideoCodecType::kVideoCodecGeneric != type) {
        switch (type) {
            case webrtc::kVideoCodecVP8:
            case webrtc::kVideoCodecVP9:
            case webrtc::kVideoCodecAV1:
                break;
            default:
                break;
        }
        if (@available(macOS 10.9, *)) {
            static std::once_flag registerProfessionalVideoWorkflowVideoEncoders;
            std::call_once(registerProfessionalVideoWorkflowVideoEncoders, VTRegisterProfessionalVideoWorkflowVideoEncoders);
        }
        if (webrtc::kVideoCodecH264 == type) {
            CFStringRef profile = nullptr;
            if (const auto profileLevelId = webrtc::ParseSdpForH264ProfileLevelId(parameters)) {
                profile = VTH264Encoder::extractProfile(profileLevelId.value());
                if (auto compressor = createCompressor(codecTypeH264(), 1280, 720, profile)) {
                    if (compressor.hardwareAccelerated()) {
                        status = CodecStatus::SupportedHardware;
                    }
                    else {
                        status = CodecStatus::SupportedSoftware;
                    }
                }
            }
        }
    }
    return status;
}

} // namespace LiveKitCpp


namespace
{

VTEncoderSession createCompressor(CMVideoCodecType codecType,
                                  int32_t width, int32_t height,
                                  CFStringRef profile)
{
    auto session = VTEncoderSession::create(width, height, codecType);
    if (session) {
        if (profile && !session.value().setProfileLevel(profile).ok()) {
            return {};
        }
    }
    return session.moveValue();
}

}
#endif
