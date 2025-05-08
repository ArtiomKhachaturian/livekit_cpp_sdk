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
#pragma once // VTH264Decoder.h
#ifndef USE_OPEN_H264_DECODER
#include "VTDecoder.h"
#include <CoreMedia/CMMemoryPool.h>
#include <memory>

namespace webrtc {
struct SdpVideoFormat;
}

namespace LiveKitCpp
{

class VTH264Decoder : public VTDecoder
{
public:
    static std::unique_ptr<webrtc::VideoDecoder> create(const webrtc::SdpVideoFormat& format);
protected:
    VTH264Decoder(const webrtc::SdpVideoFormat& format);
    // impl. of VTDecoder
    CMVideoFormatDescriptionRef createVideoFormat(const webrtc::EncodedImage& inputImage) const final;
    CMSampleBufferRef createSampleBuffer(const webrtc::EncodedImage& inputImage,
                                         CMVideoFormatDescriptionRef format) const final;
private:
    CMMemoryPoolRef memoryPoolRef() const;
};

} // namespace LiveKitCpp
#endif
