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
#ifdef USE_PLATFORM_ENCODERS
#include "H264BitstreamParser.h"
#include "MFVideoEncoder.h"
#include <api/video_codecs/h264_profile_level_id.h>

namespace LiveKitCpp 
{

class MFH264Encoder : public MFVideoEncoder
{
public:
    static std::unique_ptr<webrtc::VideoEncoder> create(const webrtc::SdpVideoFormat& format);
    // overrides of MFVideoEncoder
    int32_t InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings) final;
protected:
    CompletionStatusOr<webrtc::EncodedImage> createEncodedImage(bool keyFrame, IMFMediaBuffer* data,
                                                                const std::optional<UINT32>& encodedQp) final;
private:
    MFH264Encoder(const webrtc::SdpVideoFormat& format,
                  webrtc::H264PacketizationMode packetizationMode,
                  std::optional<webrtc::H264ProfileLevelId> profileLevelId);
private:
    const std::optional<webrtc::H264ProfileLevelId> _profileLevelId;
    H264BitstreamParser _h264BitstreamParser;
};

} // namespace LiveKitCpp
#endif