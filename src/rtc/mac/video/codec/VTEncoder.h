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
#pragma once
#ifdef USE_PLATFORM_ENCODERS
#include "CFAutoRelease.h"
#include "EncodedImageBuffer.h"
#include "VTEncoderSession.h"
#include "VTEncoderSessionCallback.h"
#include "VideoEncoder.h"
#include <optional>

namespace webrtc {
class ColorSpace;
}

namespace LiveKitCpp
{

class CFMemoryPool;
class VideoFrameBufferPoolSource;

class VTEncoder : public VideoEncoder, private VTEncoderSessionCallback
{
public:
    ~VTEncoder() override;
    // overrides of GenericCodec<>
    bool hardwareAccelerated() const override;
    // overrides of VideoEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) override;
    int32_t Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes) override;
    EncoderInfo GetEncoderInfo() const override;
protected:
    VTEncoder(const webrtc::SdpVideoFormat& format,
              webrtc::CodecSpecificInfo codecSpecificInfo,
              const std::shared_ptr<CFMemoryPool>& memoryPool = {});
    virtual CompletionStatus configureCompressionSession(VTEncoderSession* session);
    virtual MaybeEncodedImageBuffer createEncodedImageFromSampleBuffer(CMSampleBufferRef sampleBuffer,
                                                                       bool isKeyFrame,
                                                                       const CFMemoryPool* memoryPool = nullptr) = 0;
    virtual int lastQp() const { return -1; }
    // overrides of VideoEncoder
    void destroySession() override;
    CompletionStatus setEncoderBitrate(uint32_t bitrateBps) override;
    CompletionStatus setEncoderFrameRate(uint32_t frameRate) override;
private:
    CompletionStatusOr<VTEncoderSourceFrame> createSourceFrame(const webrtc::VideoFrame& frame) const;
    CompletionStatus createSession(int32_t width, int32_t height);
    // impl. of VTEncoderSessionCallback
    void onEncodedImage(VTEncoderSourceFrame frame, VTEncodeInfoFlags infoFlags, CMSampleBufferRef sampleBuffer) final;
    void onError(CompletionStatus error, bool fatal) final;
private:
    const std::shared_ptr<CFMemoryPool> _memoryPool;
    const std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    VTEncoderSession _session;
};

} // namespace LiveKitCpp
#endif
