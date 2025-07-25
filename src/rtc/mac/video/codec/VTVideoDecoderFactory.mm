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
#ifdef USE_PLATFORM_DECODERS
#include "VideoUtils.h"
#include "H264Utils.h"
#include "VTH264Decoder.h"
#import <CoreMedia/CoreMedia.h>

namespace LiveKitCpp
{

VTVideoDecoderFactory::VTVideoDecoderFactory()
    : _h264Formats(H264Utils::platformEncoderFormats())
{
}

std::unique_ptr<webrtc::VideoDecoder> VTVideoDecoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    if (auto decoder = VTH264Decoder::create(format)) {
        return decoder;
    }
    return {};
}

webrtc::VideoDecoderFactory::CodecSupport VTVideoDecoderFactory::
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
    if (webrtc::VideoCodecType::kVideoCodecGeneric != type) {
        if (@available(macOS 10.9, *)) {
            static std::once_flag registerProfessionalVideoWorkflowVideoDecoders;
            std::call_once(registerProfessionalVideoWorkflowVideoDecoders, []() {
                VTRegisterProfessionalVideoWorkflowVideoDecoders();
                //VTRegisterSupplementalVideoDecoderIfAvailable(codecTypeAV1());
                //VTRegisterSupplementalVideoDecoderIfAvailable(codecTypeVP9());
                VTRegisterSupplementalVideoDecoderIfAvailable(codecTypeH264());
            });
        }
        switch (type) {
            /*case webrtc::kVideoCodecVP9:
                if (VTIsHardwareDecodeSupported(codecTypeVP9())) {
                    return CodecStatus::SupportedMixed;
                }
                break;
            case webrtc::kVideoCodecAV1:
                if (VTIsHardwareDecodeSupported(codecTypeAV1())) {
                    return CodecStatus::SupportedMixed;
                }
                break;*/
            case webrtc::kVideoCodecH264:
                if (VTIsHardwareDecodeSupported(codecTypeH264())) {
                    return CodecStatus::SupportedMixed;
                }
                return CodecStatus::SupportedSoftware;
            default:
                break;
        }
    }
    return CodecStatus::NotSupported;
}

} // namespace LiveKitCpp
#endif
