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
#pragma once // MFH264Encoder.h
#ifndef USE_OPEN_H264_ENCODER
#include "H264BitstreamParser.h"
#include "MFVideoEncoder.h"
#include <api/video_codecs/h264_profile_level_id.h>
#include <modules/video_coding/codecs/h264/include/h264_globals.h>

namespace webrtc {
struct SdpVideoFormat;
}

namespace LiveKitCpp 
{

class MFH264Encoder : public MFVideoEncoder
{
    class NaluHeader;
    class H264EncoderBuffer;
public:
    ~MFH264Encoder() override;
    static std::unique_ptr<webrtc::VideoEncoder> create(const webrtc::SdpVideoFormat& format);
    // overrides of MFVideoEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) final;
protected:
    MFH264Encoder(const webrtc::SdpVideoFormat& format,
                  std::optional<webrtc::H264ProfileLevelId> profileLevelId,
                  webrtc::H264PacketizationMode packetizationMode);
    // overrides of MFVideoEncoder
    CompletionStatus processMpegHeaderData(std::vector<BYTE> naluHeaderData) final;
    CompletionStatusOr<MFVideoEncoderPipeline> createPipeline(UINT32 width, UINT32 height) final;
    CompletionStatusOr<webrtc::EncodedImage> createEncodedImage(bool keyFrame, 
                                                                const CComPtr<IMFMediaBuffer>& data,
                                                                const std::optional<UINT32>& encodedQp) final;

private:
    static std::optional<eAVEncH264VProfile> toMFH264(webrtc::H264Profile profile);
    static std::optional<eAVEncH264VLevel> toMFH264(webrtc::H264Level level);

private:
    const std::optional<webrtc::H264ProfileLevelId> _profileLevelId;
    std::unique_ptr<NaluHeader> _naluHeader;
    H264BitstreamParser _h264BitstreamParser;
    webrtc::VideoCodecH264 _h264params = {};
};

} // namespace LiveKitCpp
#endif