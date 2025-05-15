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
#include "MFH264Encoder.h"
#ifndef USE_OPEN_H264_ENCODER
#include "H264Utils.h"
#include <common_video/h264/h264_common.h>

namespace LiveKitCpp 
{

MFH264Encoder::MFH264Encoder(const webrtc::SdpVideoFormat& format,
                             webrtc::H264PacketizationMode packetizationMode,
                             std::optional<webrtc::H264ProfileLevelId> profileLevelId)
    : MFVideoEncoder(format, H264Utils::createCodecInfo(packetizationMode))
    , _profileLevelId(std::move(profileLevelId))
{
}

std::unique_ptr<webrtc::VideoEncoder> MFH264Encoder::create(const webrtc::SdpVideoFormat& format)
{
    std::unique_ptr<webrtc::VideoEncoder> encoder;
    if (H264Utils::formatMatched(format)) {
        const auto status = encoderStatus(format);
        if (CodecStatus::NotSupported != status) {
            auto profileLevelId = webrtc::ParseSdpForH264ProfileLevelId(format.parameters);
            const auto packetizationMode = H264Utils::packetizationMode(format);
            encoder.reset(new MFH264Encoder(format, packetizationMode, std::move(profileLevelId)));
        }
    }
    return encoder;
}

int32_t MFH264Encoder::InitEncode(const webrtc::VideoCodec* codecSettings, const Settings& encoderSettings)
{
    int32_t result = WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    if (codecSettings) {
        if (_profileLevelId.has_value()) {
            auto h264settings = H264Utils::map(*codecSettings, _profileLevelId->level);
            return MFVideoEncoder::InitEncode(&h264settings, encoderSettings);
        }
        return MFVideoEncoder::InitEncode(codecSettings, encoderSettings);
    }
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
}

CompletionStatusOr<webrtc::EncodedImage> MFH264Encoder::createEncodedImage(bool keyFrame,
                                                                           IMFMediaBuffer* data,
                                                                           const std::optional<UINT32>& encodedQp)
{
    auto result = MFVideoEncoder::createEncodedImage(keyFrame, data, encodedQp);
    if (result && !encodedQp) {
        _h264BitstreamParser.parseForSliceQp(result->GetEncodedData());
        result->qp_ = _h264BitstreamParser.lastSliceQp();
    }
    return result;
}

} // namespace LiveKitCpp
#endif