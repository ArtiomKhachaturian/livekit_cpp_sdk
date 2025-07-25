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
#include "VTH264Decoder.h"
#ifdef USE_PLATFORM_DECODERS
#include "H264Utils.h"
#include "VideoUtils.h"
#include "CFMemoryPool.h"
#include <components/video_codec/nalu_rewriter.h>

namespace LiveKitCpp
{

VTH264Decoder::VTH264Decoder(const webrtc::SdpVideoFormat& format)
    : VTDecoder(format)
{
}

std::unique_ptr<webrtc::VideoDecoder> VTH264Decoder::create(const webrtc::SdpVideoFormat& format)
{
    std::unique_ptr<webrtc::VideoDecoder> decoder;
    if (H264Utils::formatMatched(format)) {
        const auto status = decoderStatus(format);
        if (CodecStatus::NotSupported != status) {
            decoder.reset(new VTH264Decoder(format));
        }
    }
    return decoder;
}

CMVideoFormatDescriptionRef VTH264Decoder::createVideoFormat(const webrtc::EncodedImage& inputImage) const
{
    if (const auto encodedBuffer = inputImage.GetEncodedData()) {
        return webrtc::CreateVideoFormatDescription(encodedBuffer->data(), encodedBuffer->size());
    }
    return nullptr;
}

CMSampleBufferRef VTH264Decoder::createSampleBuffer(const webrtc::EncodedImage& inputImage,
                                                    CMVideoFormatDescriptionRef format) const
{
    if (const auto encodedBuffer = inputImage.GetEncodedData()) {
        CMSampleBufferRef sampleBuffer = nullptr;
        if (webrtc::H264AnnexBBufferToCMSampleBuffer(encodedBuffer->data(), encodedBuffer->size(),
                                                     format, &sampleBuffer, memoryPoolRef())) {
            return sampleBuffer;
        }
    }
    return nullptr;
}

CMMemoryPoolRef VTH264Decoder::memoryPoolRef() const
{
    if (const auto& mp = memoryPool()) {
        return mp->pool();
    }
    return nullptr;
}

} // namespace LiveKitCpp
#endif
