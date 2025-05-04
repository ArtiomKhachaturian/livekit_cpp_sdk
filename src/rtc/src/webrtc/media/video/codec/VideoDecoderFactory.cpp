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
#include "VideoDecoderFactory.h"
#include "VideoUtils.h"
#include <api/video_codecs/video_decoder_factory_template.h>
#include <api/video_codecs/video_decoder_factory_template_dav1d_adapter.h>  // nogncheck
#include <api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_open_h264_adapter.h>  // nogncheck
#include <rtc_base/logging.h>

namespace {

using Factory = webrtc::VideoDecoderFactoryTemplate<webrtc::LibvpxVp8DecoderTemplateAdapter,
                                                    //webrtc::OpenH264DecoderTemplateAdapter,
                                                    webrtc::Dav1dDecoderTemplateAdapter,
                                                    webrtc::LibvpxVp9DecoderTemplateAdapter>;
}

namespace LiveKitCpp
{

VideoDecoderFactory::VideoDecoderFactory()
    : _defaultFallback(std::make_unique<Factory>())
{
}

std::vector<webrtc::SdpVideoFormat> VideoDecoderFactory::GetSupportedFormats() const
{
    return mergeFormats(_defaultFallback.get(), customFormats());
}

std::unique_ptr<webrtc::VideoDecoder> VideoDecoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    std::unique_ptr<webrtc::VideoDecoder> decoder;
    if (const auto originalFormat = webrtc::FuzzyMatchSdpVideoFormat(GetSupportedFormats(), format)) {
        decoder = customDecoder(env, originalFormat.value());
        if (!decoder) {
            decoder = defaultDecoder(env, originalFormat.value());
        }
        if (!decoder) {
            RTC_LOG(LS_ERROR) << "Decoder for video format [" << originalFormat.value() << "] was not found";
        }
    }
    else {
        RTC_LOG(LS_ERROR) << "Requested video format [" << format << "] is not suitable for decoder factory";
    }
    return decoder;
}

webrtc::VideoDecoderFactory::CodecSupport VideoDecoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format, bool referenceScaling) const
{
    auto support = webrtc::VideoDecoderFactory::QueryCodecSupport(format, referenceScaling);
    if (support.is_supported) {
        support.is_power_efficient = powerEfficient(format);
    }
    return support;
}

std::unique_ptr<webrtc::VideoDecoder> VideoDecoderFactory::
    defaultDecoder(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) const
{
    return _defaultFallback->Create(env, format);
}

std::unique_ptr<webrtc::VideoDecoder> VideoDecoderFactory::
    customDecoder(const webrtc::Environment& /*env*/, const webrtc::SdpVideoFormat& /*format*/)
{
    return nullptr;
}

bool VideoDecoderFactory::powerEfficient(const webrtc::SdpVideoFormat& format) const
{
    return maybeHardwareAccelerated(decoderStatus(format));
}

} // namespace LiveKitCpp
