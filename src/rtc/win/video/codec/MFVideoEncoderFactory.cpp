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
#include "MFVideoEncoderFactory.h"
#ifdef USE_PLATFORM_ENCODERS
#include "CodecStatus.h"
#include "H264Utils.h"
#include "MFH264Encoder.h"

namespace LiveKitCpp
{

MFVideoEncoderFactory::MFVideoEncoderFactory()
    : _h264Formats(H264Utils::platformEncoderFormats())
{
}

std::unique_ptr<webrtc::VideoEncoder> MFVideoEncoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    if (auto h264 = MFH264Encoder::create(format)) {
        return h264;
    }
    return {};
}

webrtc::VideoEncoderFactory::CodecSupport MFVideoEncoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format, std::optional<std::string> scalabilityMode) const
{
    auto support = webrtc::VideoEncoderFactory::QueryCodecSupport(format, scalabilityMode);
    if (support.is_supported) {
        support.is_power_efficient = maybeHardwareAccelerated(encoderStatus(format));
    }
    return support;
}

CodecStatus platformEncoderStatus(webrtc::VideoCodecType type, const webrtc::CodecParameterMap& /*parameters*/)
{
    if (webrtc::VideoCodecType::kVideoCodecH264 == type) {
        return CodecStatus::SupportedHardware;
    }
    return CodecStatus::NotSupported;
}

} // namespace LiveKitCpp
#endif