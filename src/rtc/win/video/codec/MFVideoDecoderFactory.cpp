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
#include "MFVideoDecoderFactory.h"
#ifdef USE_PLATFORM_DECODERS
#include "CodecStatus.h"
#include "MFVideoDecoderPipeline.h"
#include "MFH264Decoder.h"
#include "VideoUtils.h"
#include "H264Utils.h"

namespace
{

using namespace LiveKitCpp;

CodecStatus checkDecoder(webrtc::VideoCodecType codecType, UINT32 width = 1280U, UINT32 height = 1024U);

}

namespace LiveKitCpp
{

MFVideoDecoderFactory::MFVideoDecoderFactory()
    : _h264Formats(H264Utils::platformDecoderFormats())
{
}

std::unique_ptr<webrtc::VideoDecoder> MFVideoDecoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    if (H264Utils::formatMatched(format)) {
        return std::make_unique<MFH264Decoder>(format);
    }
    return {};
}

webrtc::VideoDecoderFactory::CodecSupport MFVideoDecoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format, bool referenceScaling) const
{
    auto support = webrtc::VideoDecoderFactory::QueryCodecSupport(format, referenceScaling);
    if (support.is_supported) {
        support.is_power_efficient = maybeHardwareAccelerated(decoderStatus(format));
    }
    return support;
}

CodecStatus platformDecoderStatus(webrtc::VideoCodecType type, const webrtc::CodecParameterMap&)
{
    if (webrtc::VideoCodecType::kVideoCodecH264 == type) {
        return checkDecoder(type);
    }
    return CodecStatus::NotSupported;
}

} // namespace LiveKitCpp

namespace
{

CodecStatus checkDecoder(webrtc::VideoCodecType codecType, UINT32 width, UINT32 height)
{
    const auto pipeline = MFVideoDecoderPipeline::create(codecType, width, height);
    if (pipeline) {
        if (pipeline->hardwareAccellerated()) {
            return CodecStatus::SupportedMixed;
        }
        return CodecStatus::SupportedSoftware;
    }
    return CodecStatus::NotSupported;
}

}
#endif