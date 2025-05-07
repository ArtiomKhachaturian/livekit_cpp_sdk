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
#pragma once // MFVideoEncoder.h
#include "VideoEncoder.h"
#include "MFVideoEncoderPipeline.h"
#include <codecapi.h>
#include <mfobjects.h>
#include <queue>

namespace LiveKitCpp 
{

class MFVideoEncoder : public VideoEncoder
{
    struct FrameInfo
    {
        int64_t _renderTimeMs = 0ULL;
        int64_t _timestampUs = 0LL;
        uint32_t _timestampRtp = 0U; // ms
        int _width = 0;
        int _height = 0;
        webrtc::VideoRotation _rotation = webrtc::VideoRotation::kVideoRotation_0;
        int64_t _startTimestampMs = 0LL;
        int64_t _finishTimestampMs = 0LL;
    };
public:
    ~MFVideoEncoder() override;
    // overrides of VideoEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) override;
    int32_t Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes) override;
    EncoderInfo GetEncoderInfo() const override;
protected:
    MFVideoEncoder(const webrtc::SdpVideoFormat& format, webrtc::CodecSpecificInfo codecSpecificInfo);
    static std::optional<UINT32> encoderQp(const CComPtr<IMFSample>& sample);
    virtual CompletionStatus processMpegHeaderData(std::vector<BYTE> /*mpegHeaderData*/) { return {}; }
    virtual CompletionStatusOr<MFVideoEncoderPipeline> createPipeline(UINT32 width, UINT32 height);
    virtual CompletionStatusOr<webrtc::EncodedImage> createEncodedImage(bool keyFrame, 
                                                                        const CComPtr<IMFMediaBuffer>& data,
                                                                        const std::optional<UINT32>& encodedQp = std::nullopt) = 0;
    // impl. of VideoEncoder
    void destroySession() override;
    CompletionStatus setEncoderBitrate(uint32_t bitrateBps) final;
    CompletionStatus setEncoderFrameRate(uint32_t frameRate) final;
private:
    CompletionStatus fillMpegHeaderData(const MFVideoEncoderPipeline& encoder);
    CompletionStatus enqueueInput(bool keyFrame, const webrtc::VideoFrame& frame,
                                  int64_t startTimeMs, const CComPtr<IMFMediaBuffer>& inputBuffer);
    CompletionStatus acceptInputResolution(const webrtc::VideoFrame& frame);
    // return count of processed output frames
    std::optional<FrameInfo> popVideoFrameInfo(LONGLONG timestampNhs);
    CompletionStatusOr<UINT64> processOutput();
    CompletionStatus sendEncoded(const CComPtr<IMFMediaBuffer>& data, bool keyFrame,
                                 const FrameInfo& frameInfo,
                                 const std::optional<UINT32>& encodedQp = std::nullopt);
private:
    MFVideoEncoderPipeline _pipeline;
    UINT32 _currentWidth = 0U;
    UINT32 _currentHeight = 0U;
    bool _lastFrameDropped = false;
    // key - timestamp NHS
    std::queue<std::pair<LONGLONG, FrameInfo>> _attributeQueue;
};

} // namespace LiveKitCpp