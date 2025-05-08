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
#include "VideoUtils.h"
#include <api/video_codecs/video_encoder_factory_template.h>
#include <api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h>  // nogncheck
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h>
#ifdef USE_OPEN_H264_ENCODER
#include <api/video_codecs/video_encoder_factory_template_open_h264_adapter.h>  // nogncheck
#endif
#include <rtc_base/logging.h>

namespace {

using Factory = webrtc::VideoEncoderFactoryTemplate<webrtc::LibvpxVp8EncoderTemplateAdapter,
#ifdef USE_OPEN_H264_ENCODER
                                                    webrtc::OpenH264EncoderTemplateAdapter,
#endif
                                                    webrtc::LibaomAv1EncoderTemplateAdapter,
                                                    webrtc::LibvpxVp9EncoderTemplateAdapter>;
}

namespace LiveKitCpp
{

VideoEncoderFactory::VideoEncoderFactory()
    : _defaultFallback(std::make_unique<Factory>())
{
}

std::vector<webrtc::SdpVideoFormat> VideoEncoderFactory::GetSupportedFormats() const
{
    return mergeFormats(_defaultFallback.get(), customFormats());
}

std::unique_ptr<webrtc::VideoEncoder> VideoEncoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    std::unique_ptr<webrtc::VideoEncoder> encoder;
    if (const auto originalFormat = webrtc::FuzzyMatchSdpVideoFormat(GetSupportedFormats(), format)) {
        encoder = customEncoder(env, originalFormat.value());
        if (!encoder) {
            encoder = defaultEncoder(env, originalFormat.value());
        }
        if (!encoder) {
            RTC_LOG(LS_ERROR) << "Encoder for video format [" << originalFormat.value() << "] was not found";
        }
    }
    else {
        RTC_LOG(LS_ERROR) << "Requested video format [" << format << "] is not suitable for encoder factory";
    }
    return encoder;
}

webrtc::VideoEncoderFactory::CodecSupport VideoEncoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format, std::optional<std::string> scalabilityMode) const
{
    auto support = webrtc::VideoEncoderFactory::QueryCodecSupport(format, scalabilityMode);
    if (support.is_supported) {
        support.is_power_efficient = powerEfficient(format);
    }
    return support;
}

std::unique_ptr<webrtc::VideoEncoder> VideoEncoderFactory::
    defaultEncoder(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const
{
    return _defaultFallback->Create(env, format);
}

std::unique_ptr<webrtc::VideoEncoder> VideoEncoderFactory::
    customEncoder(const webrtc::Environment& /*env*/, const webrtc::SdpVideoFormat& /*format*/)
{
    return nullptr;
}

bool VideoEncoderFactory::powerEfficient(const webrtc::SdpVideoFormat& format) const
{
    return maybeHardwareAccelerated(encoderStatus(format));
}

} // namespace LiveKitCpp
