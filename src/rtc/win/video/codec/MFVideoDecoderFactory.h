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
#pragma once // MFVideoDecoderFactory.h
#include "VideoDecoderFactory.h"

namespace LiveKitCpp
{

class MFVideoDecoderFactory : public VideoDecoderFactory
{
public:
#ifdef USE_OPEN_H264_DECODER
    MFVideoDecoderFactory() = default;
#else
    MFVideoDecoderFactory();
protected:
    // override of VideoDecoderFactory
    std::vector<webrtc::SdpVideoFormat> customFormats() const final;
    std::unique_ptr<webrtc::VideoDecoder> customDecoder(const webrtc::Environment& env,
                                                        const webrtc::SdpVideoFormat& format) final;
private:
    const std::vector<webrtc::SdpVideoFormat> _h264Formats;
#endif
};

} // namespace LiveKitCpp
