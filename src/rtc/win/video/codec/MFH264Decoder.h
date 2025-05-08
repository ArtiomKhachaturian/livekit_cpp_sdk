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
#pragma once // MFH264Decoder.h
#ifndef USE_OPEN_H264_DECODER
#include "MFVideoDecoder.h"
#include <common_video/h264/h264_bitstream_parser.h>

namespace LiveKitCpp 
{

class MFH264Decoder : public MFVideoDecoder
{
public:
    MFH264Decoder(const webrtc::SdpVideoFormat& format);
protected:
    std::optional<uint8_t> lastSliceQp(const webrtc::EncodedImage& inputImage) final;
private:
    webrtc::H264BitstreamParser _h264BitstreamParser;
};

} // namespace LiveKitCpp
#endif