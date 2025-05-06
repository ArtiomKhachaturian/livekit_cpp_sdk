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

namespace LiveKitCpp
{

std::vector<webrtc::SdpVideoFormat> MFVideoDecoderFactory::customFormats() const
{
    return VideoDecoderFactory::customFormats();
}

std::unique_ptr<webrtc::VideoDecoder> MFVideoDecoderFactory::
    customDecoder(const webrtc::Environment& env, const webrtc::SdpVideoFormat& format)
{
    return MFVideoDecoderFactory::customDecoder(env, format);
}

CodecStatus platformDecoderStatus(webrtc::VideoCodecType /*type*/, const webrtc::CodecParameterMap&)
{
    return CodecStatus::SupportedSoftware;
}

} // namespace LiveKitCpp
