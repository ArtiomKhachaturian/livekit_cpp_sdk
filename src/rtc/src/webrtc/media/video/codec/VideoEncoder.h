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
#pragma once // VideoEncoder.h
#include "GenericCodec.h"
#include "CodecStatus.h"
#include "SafeObj.h"
#include <atomic>
#include <modules/video_coding/include/video_codec_interface.h>


namespace LiveKitCpp
{

class VideoEncoder : public GenericCodec<webrtc::VideoEncoder>
{
    class BitrateAdjuster;
public:
    ~VideoEncoder() override;
    // minAdjustedBitratePct and maxAdjustedBitratePct are the lower and
    // upper bound outputted adjusted bitrates as a percentage of the target bitrate
    static constexpr float minAdjustedBitratePct() { return .5f; }
    static constexpr float maxAdjustedBitratePct() { return .95f; }
    // overrides of webrtc::VideoEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) override;
    int32_t Release() override;
    int32_t RegisterEncodeCompleteCallback(webrtc::EncodedImageCallback* callback) override;
    void SetRates(const RateControlParameters& parameters) override;
    EncoderInfo GetEncoderInfo() const override;
protected:
    VideoEncoder(bool hardwareAccelerated,
                 webrtc::CodecSpecificInfo codecSpecificInfo,
                 bool useTrustedBitrateController);
    // impl. of GenericCodec<>
    webrtc::VideoCodecType type() const noexcept final { return _codecSpecificInfo.codecType; }
    uint32_t currentBitrate() const { return _currentBitrate.load(std::memory_order_relaxed); }
    uint32_t minBitrateBps() const { return _minBitrate; }
    uint32_t maxBitrateBps() const { return _maxBitrate; }
    uint32_t maxFramerate() const { return _maxFramerate; }
    uint32_t currentFramerate() const { return _currentFramerate; }
    uint32_t qpMax() const { return _qpMax; }
    webrtc::VideoCodecMode mode() const { return _mode; }
    bool realtimeMode() const { return webrtc::VideoCodecMode::kRealtimeVideo == mode(); }
    bool hasEncodeCompleteCallback() const;
    void dropEncodedImage(webrtc::EncodedImageCallback::DropReason reason) const;
    void sendEncodedImage(bool keyFrame, webrtc::EncodedImage encodedImage);
    bool dropNextFrame();
    virtual void destroySession();
    virtual webrtc::RTCError setEncoderBitrate(uint32_t bitrateBps) = 0;
    virtual webrtc::RTCError setEncoderFrameRate(uint32_t frameRate) = 0;
    static bool keyFrameRequested(const std::vector<webrtc::VideoFrameType>* frameTypes);
    static void addVideoFrameBufferType(webrtc::VideoFrameBuffer::Type type, EncoderInfo& info);
private:
    void setTargetBitrate(uint32_t bps);
    void setCurrentBitrate(uint32_t bps);
    void setCurrentFramerate(double framerateFps);
private:
    const webrtc::CodecSpecificInfo _codecSpecificInfo;
    const std::unique_ptr<BitrateAdjuster> _bitrateAdjuster;
    Bricks::SafeObj<webrtc::EncodedImageCallback*> _callback = nullptr;
    std::atomic<uint32_t> _currentFramerate = 0U;
    std::atomic<uint32_t> _maxFramerate = 0U;
    // bitrates in BPS
    std::atomic<uint32_t> _currentBitrate = 0U;
    std::atomic<uint32_t> _minBitrate = 0U;
    std::atomic<uint32_t> _maxBitrate = 0U;
    std::atomic<uint32_t> _qpMax = 100; // quantizer quality
    std::atomic<webrtc::VideoCodecMode> _mode = webrtc::VideoCodecMode::kRealtimeVideo;
    std::atomic_bool _dropNextFrame = false;
};

} // namespace LiveKitCpp
