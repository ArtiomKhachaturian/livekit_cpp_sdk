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
#ifdef USE_PLATFORM_DECODERS
#include "VideoDecoder.h"
#include "CompletionStatusOr.h"
#include "MFSampleTimeLine.h"
#include <atlbase.h> //CComPtr support
#include <mftransform.h>
#include <mfobjects.h>
#include <optional>

namespace LiveKitCpp 
{

class VideoFrameBufferPoolSource;

class MFVideoDecoder : public VideoDecoder
{
    class NV12MisalignedBuffer;
public:
    MFVideoDecoder(const webrtc::SdpVideoFormat& format);
    ~MFVideoDecoder() override;
    // overrides of GenericCodec<>
    bool hardwareAccelerated() const override;
    // overrides of VideoDecoder
    bool Configure(const Settings& settings) final;
    int32_t Decode(const webrtc::EncodedImage& inputImage, bool missingFrames, int64_t renderTimeMs) override;
    webrtc::VideoDecoder::DecoderInfo GetDecoderInfo() const override;
protected:
    CompletionStatus destroySession() override;
    virtual std::optional<uint8_t> lastSliceQp(const webrtc::EncodedImage&) { return std::nullopt; }
private:
    static CompletionStatusOrComPtr<IMFMediaType> createInputMedia(const GUID& format,
                                                                   UINT32 width = 0U, UINT32 height = 0U,
                                                                   UINT32 fps = 0U);
    static CompletionStatus configureOutputMedia(const CComPtr<IMFTransform>& decoder,
                                                 const GUID& format, bool& typeFound);
    static bool videoAccelerated(webrtc::VideoCodecType type, const CComPtr<IMFAttributes>& attributes);
    CompletionStatus enqueueFrame(const webrtc::EncodedImage& inputImage, bool missingFrames);
    CompletionStatus flushFrames(const webrtc::EncodedImage& inputImage);
    CompletionStatus sendDecodedSample(const webrtc::EncodedImage& inputImage, const CComPtr<IMFSample>& sample);
    CompletionStatusOr<DWORD> outputStatus() const;
private:
    const std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    CComPtr<IMFTransform> _decoder;
    bool _requestKeyFrame = false;
    MFSampleTimeLine _inputFramesTimeline;
};

} // namespace LiveKitCpp
#endif