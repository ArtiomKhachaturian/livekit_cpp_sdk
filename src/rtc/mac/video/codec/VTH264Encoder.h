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
#pragma once // VTH264Encoder.h
#ifndef USE_OPEN_H264_ENCODER
#include "MacH264BitstreamParser.h"
#include "VTEncoder.h"
#include <api/video_codecs/h264_profile_level_id.h>
#include <modules/video_coding/codecs/h264/include/h264_globals.h>

namespace webrtc {
struct SdpVideoFormat;
}

namespace LiveKitCpp
{

class VTEncoderSession;

class VTH264Encoder : public VTEncoder
{
public:
    ~VTH264Encoder() override;
    static std::unique_ptr<webrtc::VideoEncoder> create(const webrtc::SdpVideoFormat& format);
    static CFStringRef extractProfile(const webrtc::H264ProfileLevelId& profileLevelId);
    // overrides of VTEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) final;
    int32_t Release() final;
    EncoderInfo GetEncoderInfo() const final;
protected:
    VTH264Encoder(const webrtc::SdpVideoFormat& format, webrtc::H264PacketizationMode mode);
    // overrides of VTEncoder
    void destroySession() final;
    CompletionStatus configureCompressionSession(VTEncoderSession* session) final;
    MaybeEncodedImageBuffer createEncodedImageFromSampleBuffer(CMSampleBufferRef sampleBuffer,
                                                               bool isKeyFrame,
                                                               const CFMemoryPool* memoryPool = nullptr) final;
    int lastQp() const final { return _h264BitstreamParser.lastSliceQp(); }
private:
    const std::optional<webrtc::H264ProfileLevelId> _profileLevelId;
    H264BitstreamParser _h264BitstreamParser;
    // required for prediction of cached frame size (it's a max frame size)
    size_t _outputBufferCacheSize = 0UL;
    int _keyFrameInterval = 0;
};

} // namespace LiveKitCpp
#endif
