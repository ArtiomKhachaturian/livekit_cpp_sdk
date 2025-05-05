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
#include "VideoDecoder.h"
#include <algorithm>

namespace LiveKitCpp
{

VideoDecoder::VideoDecoder(const webrtc::SdpVideoFormat& format)
    : GenericCodec<webrtc::VideoDecoder>(format)
{
}

int VideoDecoder::maxDecodingThreads(int width, int height, int maxCores)
{
    if (width > 0 && height > 0 && maxCores > 0) {
        // We want to use multithreading when decoding high resolution videos. But
        // not too many in order to avoid overhead when many stream are decoded
        // concurrently.
        // Set 2 thread as target for 1280x720 pixel count, and then scale up
        // linearly from there - but cap at physical core count.
        // For common resolutions this results in:
        // 1 for 360p
        // 2 for 720p
        // 4 for 1080p
        // 8 for 1440p
        // 18 for 4K
        const auto effectiveCores = std::max<int>(1, 2 * (width * height) / (1280 * 720));
        return std::min(effectiveCores, maxCores);
    }
    return 0;
}

bool VideoDecoder::Configure(const Settings& settings)
{
    if (type() == settings.codec_type()) {
        _bufferPoolSize = settings.buffer_pool_size().value_or(0);
        return true;
    }
    return false;
}

int32_t VideoDecoder::RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback)
{
    _callback = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t VideoDecoder::Release()
{
    destroySession();
    return RegisterDecodeCompleteCallback(nullptr);
}

webrtc::VideoDecoder::DecoderInfo VideoDecoder::GetDecoderInfo() const
{
    auto decoderInfo = GenericCodec<webrtc::VideoDecoder>::GetDecoderInfo();
    decoderInfo.implementation_name = ImplementationName();
    decoderInfo.is_hardware_accelerated = hardwareAccelerated();
    return decoderInfo;
}

void VideoDecoder::sendDecodedImage(webrtc::VideoFrame& decodedImage,
                                    std::optional<int32_t> decodeTimeMs,
                                    std::optional<uint8_t> qp) const
{
    _callback.invoke<DecodedMethodType>(&webrtc::DecodedImageCallback::Decoded,
                                        decodedImage,
                                        std::move(decodeTimeMs),
                                        std::move(qp));
}

} // namespace LiveKitCpp
