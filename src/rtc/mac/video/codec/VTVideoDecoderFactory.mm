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
#include "VTVideoDecoderFactory.h"
#include "VideoUtils.h"
#include "VTH264Decoder.h"

namespace LiveKitCpp
{

webrtc::VideoDecoderFactory::CodecSupport VTVideoDecoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format,
                      bool referenceScaling) const
{
    auto support = VideoDecoderFactory::QueryCodecSupport(format, referenceScaling);
    if (support.is_supported && CodecStatus::SupportedHardware == VTDecoder::checkDecoderSupport(format)) {
        support.is_power_efficient = true;
    }
    return support;
}

std::unique_ptr<webrtc::VideoDecoder> VTVideoDecoderFactory::Create(const webrtc::Environment& env,
                                                                    const webrtc::SdpVideoFormat& format)
{
    if (isH264VideoFormat(format)) {
        auto decoder = VTH264Decoder::create(format);
        if (decoder) {
            return decoder;
        }
    }
    return VideoDecoderFactory::Create(env, format);
}

} // namespace LiveKitCpp
