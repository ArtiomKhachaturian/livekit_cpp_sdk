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
#include "CodecStatus.h"
#include "H264Utils.h"
#include "MFH264Encoder.h"
#include <api/transport/bitrate_settings.h>

namespace
{

using namespace LiveKitCpp;

CodecStatus checkEncoder(webrtc::VideoCodecType codecType, 
                         UINT32 width = 1280U, UINT32 height = 1024U);

}


namespace LiveKitCpp
{

std::unique_ptr<webrtc::VideoEncoder> MFVideoEncoderFactory::
    customEncoder(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    if (auto h264 = MFH264Encoder::create(format)) {
        return h264;
    }
    return VideoEncoderFactory::customEncoder(env, format);
}

std::vector<webrtc::SdpVideoFormat> MFVideoEncoderFactory::customFormats() const
{
    return H264Utils::supportedFormats(true);
}

CodecStatus platformEncoderStatus(webrtc::VideoCodecType type, const webrtc::CodecParameterMap& parameters)
{
    if (webrtc::VideoCodecType::kVideoCodecH264 == type) {
        return checkEncoder(type);
    }
    return CodecStatus::SupportedSoftware;
}

} // namespace LiveKitCpp

namespace
{

CodecStatus checkEncoder(webrtc::VideoCodecType codecType, UINT32 width, UINT32 height)
{
    const auto bitrate = webrtc::BitrateConstraints{}.start_bitrate_bps;
    auto pipeline = MFVideoEncoderPipeline::create(true, codecType, width, height, 30, bitrate);
    if (pipeline) {
        if (pipeline->hardwareAccellerated() || pipeline->directXAccelerationSupported()) {
            return CodecStatus::SupportedMixed;
        }
        return CodecStatus::SupportedSoftware;
    }
    return CodecStatus::NotSupported;
}

}