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
#include <absl/strings/match.h>
#include <api/environment/environment.h>
#include <api/video_codecs/av1_profile.h>
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/video_codec.h>
#include <media/base/codec.h>
#include <media/base/media_constants.h>
#include <modules/video_coding/codecs/h264/include/h264.h>
#include <modules/video_coding/codecs/vp8/include/vp8.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>
#include <modules/video_coding/codecs/av1/dav1d_decoder.h>
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <system_wrappers/include/field_trial.h>

namespace LiveKitCpp
{

std::vector<webrtc::SdpVideoFormat> VideoDecoderFactory::GetSupportedFormats() const
{
    std::vector<webrtc::SdpVideoFormat> formats;
    formats.push_back(webrtc::SdpVideoFormat::VP8());
    for (const auto& format : webrtc::SupportedVP9DecoderCodecs()) {
        formats.push_back(format);
    }
    for (const auto& h264_format : webrtc::SupportedH264DecoderCodecs()) {
        formats.push_back(h264_format);
    }
    formats.push_back(webrtc::SdpVideoFormat::AV1Profile0());
    formats.push_back(webrtc::SdpVideoFormat::AV1Profile1());
    return formats;
}

webrtc::VideoDecoderFactory::CodecSupport VideoDecoderFactory::
    QueryCodecSupport(const webrtc::SdpVideoFormat& format, bool referenceScaling) const {
    // Query for supported formats and check if the specified format is supported.
    // Return unsupported if an invalid combination of format and
    // reference_scaling is specified.
    if (referenceScaling) {
        auto codec = webrtc::PayloadStringToCodecType(format.name);
        if (codec != webrtc::kVideoCodecVP9 && codec != webrtc::kVideoCodecAV1) {
            return {/*is_supported=*/false, /*is_power_efficient=*/false};
        }
    }
    CodecSupport codec_support;
    codec_support.is_supported = format.IsCodecInList(GetSupportedFormats());
    return codec_support;
}

std::unique_ptr<webrtc::VideoDecoder> VideoDecoderFactory::
    Create(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format) {
    if (!format.IsCodecInList(GetSupportedFormats())) {
        RTC_LOG(LS_WARNING) << "Trying to create decoder for unsupported format. "
                            << format.ToString();
        return nullptr;
    }
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName)) {
        return webrtc::CreateVp8Decoder(env);
    }
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName)) {
        return webrtc::VP9Decoder::Create();
    }
    if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName)) {
        return webrtc::H264Decoder::Create();
    }
    if (absl::EqualsIgnoreCase(format.name, cricket::kAv1CodecName)) {
        return webrtc::CreateDav1dDecoder();
    }
    RTC_DCHECK_NOTREACHED();
    return nullptr;
}

} // namespace LiveKitCpp
