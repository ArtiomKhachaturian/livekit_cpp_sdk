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
#pragma once // GenericCodec.h
#ifdef WEBRTC_MAC
#include "VideoUtils.h"
#endif
#include <api/rtc_error.h>
#include <api/video_codecs/video_codec.h>
#include <api/video_codecs/video_decoder.h>
#include <api/video_codecs/video_encoder.h>
#include <modules/video_coding/include/video_error_codes.h>
#include <rtc_base/logging.h>

namespace LiveKitCpp
{

template <class TCodecInterface>
class GenericCodec : public TCodecInterface
{
public:
    bool hardwareAccelerated() const { return _hardwareAccelerated; }
    virtual webrtc::VideoCodecType type() const noexcept = 0;
    virtual std::string name() const;
    static constexpr const char* mediaBackendName();
protected:
    GenericCodec(bool hardwareAccelerated);
    webrtc::RTCError log(webrtc::RTCError error, bool fatal = true);
    static constexpr bool isEncoder() { return std::is_same<webrtc::VideoEncoder, TCodecInterface>::value; }
private:
    const bool _hardwareAccelerated;
};

template <class TCodecInterface>
inline GenericCodec<TCodecInterface>::GenericCodec(bool hardwareAccelerated)
    : _hardwareAccelerated(hardwareAccelerated)
{
}

template <class TCodecInterface>
inline std::string GenericCodec<TCodecInterface>::name() const
{
#ifdef WEBRTC_MAC
    if (const auto vtType = toVTCodecType(type())) {
        return videoCodecTypeToString(vtType.value());
    }
#endif
    return webrtc::CodecTypeToPayloadString(type());
}

template <class TCodecInterface>
inline constexpr const char* GenericCodec<TCodecInterface>::mediaBackendName()
{
#ifdef WEBRTC_MAC
    return "VideoToolbox";
#elif defined(WEBRTC_WIN)
    return "MediaFoundation";
#endif
}

template <class TCodecInterface>
inline webrtc::RTCError GenericCodec<TCodecInterface>::
    log(webrtc::RTCError error, bool fatal)
{
    if (!error.ok()) {
        if (fatal) {
            RTC_LOG(LS_ERROR) << error;
        }
        else {
            RTC_LOG(LS_WARNING) << error;
        }
    }
    return error;
}

} // namespace LiveKitCpp
