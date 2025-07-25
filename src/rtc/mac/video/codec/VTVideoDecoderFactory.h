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
#pragma once // VTVideoDecoderFactory.h
#ifdef USE_PLATFORM_DECODERS
#include <api/video_codecs/video_decoder_factory.h>

namespace LiveKitCpp
{

class VTVideoDecoderFactory : public webrtc::VideoDecoderFactory
{
public:
    VTVideoDecoderFactory();
    // impl. of webrtc::VideoEncoderFactory
    std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const final { return _h264Formats; }
    std::unique_ptr<webrtc::VideoDecoder> Create(const webrtc::Environment& env,
                                                 const webrtc::SdpVideoFormat& format) final;
    CodecSupport QueryCodecSupport(const webrtc::SdpVideoFormat& format, bool referenceScaling) const final;
private:
    const std::vector<webrtc::SdpVideoFormat> _h264Formats;
};

} // namespace LiveKitCpp
#endif
