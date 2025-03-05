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
#include "VideoEncoderFactory.h"
#include <absl/types/optional.h>
#include <api/environment/environment.h>
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/video_encoder_factory.h>
#include <api/video_codecs/video_encoder_factory_template.h>
#include <api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h>  // nogncheck
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_open_h264_adapter.h>  // nogncheck

namespace {

using Factory = webrtc::VideoEncoderFactoryTemplate<webrtc::LibvpxVp8EncoderTemplateAdapter,
                                                    webrtc::OpenH264EncoderTemplateAdapter,
                                                    webrtc::LibaomAv1EncoderTemplateAdapter,
                                                    webrtc::LibvpxVp9EncoderTemplateAdapter>;
}

namespace LiveKitCpp
{

VideoEncoderFactory::VideoEncoderFactory()
    : _factory(std::make_unique<Factory>())
{
}

std::vector<webrtc::SdpVideoFormat> VideoEncoderFactory::GetSupportedFormats() const
{
    return _factory->GetSupportedFormats();
}

webrtc::VideoEncoderFactory::CodecSupport VideoEncoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format,
                      std::optional<std::string> scalabilityMode) const
{
    const auto originalFormat = webrtc::FuzzyMatchSdpVideoFormat(_factory->GetSupportedFormats(), format);
    if (originalFormat) {
        return _factory->QueryCodecSupport(originalFormat.value(), std::move(scalabilityMode));
    }
    return CodecSupport{.is_supported = false};
}

std::unique_ptr<webrtc::VideoEncoder> VideoEncoderFactory::Create(const webrtc::Environment& env,
                                                                  const webrtc::SdpVideoFormat& format)
{
    const auto originalFormat = webrtc::FuzzyMatchSdpVideoFormat(_factory->GetSupportedFormats(), format);
    if (originalFormat) {
        return _factory->Create(env, originalFormat.value());
    }
    return nullptr;
}

} // namespace LiveKitCpp
