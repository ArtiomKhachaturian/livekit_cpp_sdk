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
#ifdef USE_PLATFORM_ENCODERS
#include "MFSortedQueue.h"
#include "MFTEncodingCallback.h"
#include "MFMediaSink.h"
#include "MFSampleTimeLine.h"
#include "VideoEncoder.h"

#include <mfreadwrite.h>

#include "MFVideoEncoderPipeline.h"
#include "VideoFrameInfo.h"
#include <codecapi.h>
#include <mfobjects.h>
#include <queue>

namespace LiveKitCpp 
{

class MFVideoEncoder : public VideoEncoder
{
public:
    ~MFVideoEncoder() override;
    // overrides of GenericCodec<>
    bool hardwareAccelerated() const override;
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
    CompletionStatus destroySession() override;
    CompletionStatus setEncoderBitrate(uint32_t bitrateBps) final;
    CompletionStatus setEncoderFrameRate(uint32_t frameRate) final;
private:
    CompletionStatus fillMpegHeaderData(const MFVideoEncoderPipeline& encoder);
    CompletionStatus enqueueInput(bool keyFrame, const webrtc::VideoFrame& frame,
                                  int64_t startTimeMs, const CComPtr<IMFMediaBuffer>& inputBuffer);
    CompletionStatus acceptInputResolution(const webrtc::VideoFrame& frame);
    // return count of processed output frames
    std::optional<VideoFrameInfo> popVideoFrameInfo(LONGLONG timestampNhs);
    CompletionStatusOr<UINT64> processOutput();
    CompletionStatus sendEncoded(const CComPtr<IMFMediaBuffer>& data, bool keyFrame,
                                 const VideoFrameInfo& frameInfo,
                                 const std::optional<UINT32>& encodedQp = std::nullopt);
private:
    MFVideoEncoderPipeline _pipeline;
    UINT32 _currentWidth = 0U;
    UINT32 _currentHeight = 0U;
    bool _lastFrameDropped = false;
    // key - timestamp NHS
    std::queue<std::pair<LONGLONG, VideoFrameInfo>> _attributeQueue;
};


class VideoFrameBufferPoolSource;

class MFVideoEncoder2 : public VideoEncoder, private MFTEncodingCallback
{
    class EncodedBuffer;
public:
    MFVideoEncoder2(const webrtc::SdpVideoFormat& format, webrtc::CodecSpecificInfo codecSpecificInfo);
    bool hardwareAccelerated() const final { return true; }
    // overrides of VideoEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) override;
    int32_t Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frameTypes) override;
    EncoderInfo GetEncoderInfo() const override;
protected:
    CompletionStatus destroySession() override;
    CompletionStatus setEncoderBitrate(uint32_t bitrateBps) override;
    CompletionStatus setEncoderFrameRate(uint32_t frameRate) override;
    virtual CompletionStatusOr<webrtc::EncodedImage> createEncodedImage(bool keyFrame,
                                                                        IMFMediaBuffer* data,
                                                                        const std::optional<UINT32>& encodedQp = std::nullopt);
private:
    static std::optional<UINT32> encoderQp(IMFSample* sample);
    CComPtr<IMFSample> fromVideoFrame(const webrtc::VideoFrame& frame);
    CompletionStatus initWriter(UINT32 width, UINT32 height, UINT32 targetBps, UINT32 framerate, UINT32 maxQP);
    CompletionStatus releaseWriter();
    CompletionStatus reconfigureWriter(UINT32 newWidth, UINT32 newHeight,
                                       UINT32 newTargetBps, UINT32 newFrameRate);
    const GUID& compressedFormat() const;
    // impl. of MFTEncodingCallback
    void onEncoded(CComPtr<IMFSample> sample) final;
private:
    static constexpr int _minIntervalBetweenRateChangesMs = 5000;
    std::shared_ptr<VideoFrameBufferPoolSource> _framesPool;
    Microsoft::WRL::ComPtr<IMFSinkWriter> _sinkWriter;
    Microsoft::WRL::ComPtr<MFMediaSink> _mediaSink;
    UINT32 _width = 0U;
    UINT32 _height = 0U;
    DWORD _streamIndex = {};
    int64_t _lastRateChangeTimeMs = 0LL;
    bool _rateChangeRequested = false;
    bool _lastInputFrameDropped = false;
    uint64_t _framesCount = 0ULL;
    MFSortedQueue<VideoFrameInfo> _sampleAttributeQueue;
    MFSampleTimeLine _inputFramesTimeline;
};

} // namespace LiveKitCpp
#endif