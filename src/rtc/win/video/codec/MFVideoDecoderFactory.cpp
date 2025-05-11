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
#include "CodecStatus.h"
#include "MFVideoDecoderPipeline.h"
#ifndef USE_OPEN_H264_DECODER
#include "MFH264Decoder.h"
#include "VideoUtils.h"
#include "H264Utils.h"
#endif

namespace
{

using namespace LiveKitCpp;

CodecStatus checkDecoder(webrtc::VideoCodecType codecType, UINT32 width = 1280U, UINT32 height = 1024U);

}

namespace LiveKitCpp
{

#ifndef USE_OPEN_H264_DECODER
MFVideoDecoderFactory::MFVideoDecoderFactory()
    : _h264Formats(H264Utils::supportedFormats(false))
{
}

std::vector<webrtc::SdpVideoFormat> MFVideoDecoderFactory::customFormats() const
{
    return _h264Formats;
}

std::unique_ptr<webrtc::VideoDecoder> MFVideoDecoderFactory::
    customDecoder(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    if (H264Utils::formatMatched(format)) {
        return std::make_unique<MFH264Decoder>(format);
    }
    return VideoDecoderFactory::customDecoder(env, format);
}
#endif

CodecStatus platformDecoderStatus(webrtc::VideoCodecType type, const webrtc::CodecParameterMap&)
{
#ifndef USE_OPEN_H264_DECODER
    if (webrtc::VideoCodecType::kVideoCodecH264 == type) {
        return checkDecoder(type);
    }
#endif
    return CodecStatus::SupportedSoftware;
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