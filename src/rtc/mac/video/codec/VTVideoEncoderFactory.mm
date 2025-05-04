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
#include "VTH264Encoder.h"

namespace LiveKitCpp
{

webrtc::VideoEncoderFactory::CodecSupport VTVideoEncoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format,
                      std::optional<std::string> scalabilityMode) const
{
    auto support = VideoEncoderFactory::QueryCodecSupport(format, std::move(scalabilityMode));
    if (support.is_supported && maybeHardwareAccelerated(VideoEncoder::status(format))) {
        support.is_power_efficient = true;
    }
    return support;
}

std::unique_ptr<webrtc::VideoEncoder> VTVideoEncoderFactory::Create(const webrtc::Environment& env,
                                                                    const webrtc::SdpVideoFormat& format)
{
    if (isH264VideoFormat(format)) {
        if (auto decoder = VTH264Encoder::create(format)) {
            return decoder;
        }
    }
    return VideoEncoderFactory::Create(env, format);
}

} // namespace darkmatter::rtc
