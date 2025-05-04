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
#pragma once // H264Utils.h
#include <api/video_codecs/h264_profile_level_id.h>
#include <api/video_codecs/sdp_video_format.h>
#include <modules/video_coding/include/video_codec_interface.h>
#include <modules/video_coding/codecs/h264/include/h264_globals.h>

namespace webrtc {
enum class H264Level;
}

namespace LiveKitCpp
{

class H264Utils
{
public:
    // these thresholds deviate from the default h264 QP thresholds, as they
    // have been found to work better on devices that support VideoToolbox (APPLE)
    static constexpr int lowQpThreshold() { return 28; }
    static constexpr int highQpThreshold() { return 56; }
    static std::vector<webrtc::SdpVideoFormat> supportedFormats(bool encoder);
    // https://en.wikipedia.org/wiki/Context-adaptive_binary_arithmetic_coding
    static bool formatMatched(const webrtc::SdpVideoFormat& format);
    static bool codecNameMatched(const std::string& codecName);
    static webrtc::VideoCodec map(const webrtc::VideoCodec& settings, webrtc::H264Level level);
    static webrtc::H264PacketizationMode packetizationMode(const webrtc::SdpVideoFormat& format);
    static webrtc::H264PacketizationMode packetizationMode(const std::string_view& mode);
    static webrtc::CodecSpecificInfo createCodecInfo(const webrtc::SdpVideoFormat& format);
    static webrtc::CodecSpecificInfo createCodecInfo(webrtc::H264PacketizationMode packetizationMode);
    // https://en.wikipedia.org/wiki/Context-adaptive_binary_arithmetic_coding
    // only supported in the Main and higher profiles (but not the extended profile)
    static bool cabacIsSupported(webrtc::H264Profile profile);
};

} // namespace LiveKitCpp
