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
#pragma once // MFVideoDecoder.h
#include "VideoDecoder.h"
#include "MFVideoDecoderPipeline.h"
#include <guiddef.h>
#include <mfobjects.h>
#include <optional>

namespace LiveKitCpp 
{

class MFVideoDecoder : public VideoDecoder
{
public:
    ~MFVideoDecoder() override;
    // overrides of GenericCodec<>
    bool hardwareAccelerated() const override;
    // overrides of VideoDecoder
    bool Configure(const Settings& settings) final;
    int32_t Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) override;
protected:
    MFVideoDecoder(const webrtc::SdpVideoFormat& format);
    virtual std::optional<uint8_t> lastSliceQp(const webrtc::EncodedImage& /*inputImage*/) { return std::nullopt; }
    virtual CompletionStatus setCompressedFrameSize(UINT32 width, UINT32 height);
    void destroySession() override;
private:
    CompletionStatus enqueueFrame(const webrtc::EncodedImage& inputImage, bool missingFrames);
    CompletionStatus flushFrames(const webrtc::EncodedImage& inputImage);
    CompletionStatus sendDecodedSample(const webrtc::EncodedImage& inputImage, const CComPtr<IMFSample>& sample);
private:
    MFVideoDecoderPipeline _pipeline;
    bool _requestKeyFrame = false;
    UINT32 _currentWidth = 0U;
    UINT32 _currentHeight = 0U;
};

} // namespace LiveKitCpp