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
#include "MFH264Decoder.h"
#ifndef USE_OPEN_H264_DECODER
namespace LiveKitCpp 
{

MFH264Decoder::MFH264Decoder(const webrtc::SdpVideoFormat& format)
    : MFVideoDecoder(format)
{
}

std::optional<uint8_t> MFH264Decoder::lastSliceQp(const webrtc::EncodedImage& inputImage)
{
    _h264BitstreamParser.ParseBitstream(inputImage);
    return _h264BitstreamParser.GetLastSliceQp();
}

} // namespace LiveKitCpp
#endif