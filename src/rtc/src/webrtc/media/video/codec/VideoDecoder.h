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
#pragma once // VideoDecoder.h
#include "GenericCodec.h"
#include "CompletionStatus.h"
#include "CodecStatus.h"
#include "Listener.h"
#include <atomic>

namespace LiveKitCpp
{

class VideoDecoder : public GenericCodec<webrtc::VideoDecoder>
{
    using DecodedMethodType = void(webrtc::DecodedImageCallback::*)(webrtc::VideoFrame&,
                                                                    std::optional<int32_t>,
                                                                    std::optional<uint8_t>);
public:
    static int maxDecodingThreads(int width, int height, int maxCores);
    // impl. or overrides of GenericCodec<webrtc::VideoDecoder>
    bool Configure(const webrtc::VideoDecoder::Settings& settings) override;
    int32_t RegisterDecodeCompleteCallback(webrtc::DecodedImageCallback* callback) override;
    int32_t Release() override;
    webrtc::VideoDecoder::DecoderInfo GetDecoderInfo() const override;
    const char* ImplementationName() const override { return mediaBackendName(); }
protected:
    VideoDecoder(const webrtc::SdpVideoFormat& format);
    void sendDecodedImage(webrtc::VideoFrame& decodedImage,
                          std::optional<int32_t> decodeTimeMs = std::nullopt,
                          std::optional<uint8_t> qp = std::nullopt) const;
    bool hasDecodeCompleteCallback() const { return !_callback.empty(); }
    int bufferPoolSize() const { return _bufferPoolSize; }
    virtual CompletionStatus destroySession() { return {}; }
private:
    Bricks::Listener<webrtc::DecodedImageCallback*> _callback;
    std::atomic<int> _bufferPoolSize = 0;
};

} // namespace LiveKitCpp
