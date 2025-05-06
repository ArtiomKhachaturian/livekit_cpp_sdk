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
#include "VideoUtils.h"
#include "CompletionStatus.h"
#include <api/video_codecs/sdp_video_format.h>
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
    virtual bool hardwareAccelerated() const;
    virtual std::string name() const;
    webrtc::VideoCodecType type() const;
    static constexpr const char* mediaBackendName();
protected:
    GenericCodec(const webrtc::SdpVideoFormat& format);
    CompletionStatus logError(CompletionStatus status, bool fatal = true);
    static constexpr bool isEncoder() { return std::is_same<webrtc::VideoEncoder, TCodecInterface>::value; }
private:
    const webrtc::SdpVideoFormat _format;
};

template <class TCodecInterface>
inline GenericCodec<TCodecInterface>::GenericCodec(const webrtc::SdpVideoFormat& format)
    : _format(format)
{
}

template <class TCodecInterface>
inline bool GenericCodec<TCodecInterface>::hardwareAccelerated() const
{
    CodecStatus status = CodecStatus::NotSupported;
    if constexpr (isEncoder()) {
        status = encoderStatus(_format);
    }
    else {
        status = decoderStatus(_format);
    }
    return maybeHardwareAccelerated(status);
}

template <class TCodecInterface>
inline std::string GenericCodec<TCodecInterface>::name() const
{
#ifdef WEBRTC_MAC
    if (const auto vtType = toVTCodecType(type())) {
        return videoCodecTypeToString(vtType.value());
    }
#endif
    return _format.name;
}

template <class TCodecInterface>
inline webrtc::VideoCodecType GenericCodec<TCodecInterface>::type() const
{
    return webrtc::PayloadStringToCodecType(_format.name);
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
inline CompletionStatus GenericCodec<TCodecInterface>::
    logError(CompletionStatus status, bool fatal)
{
    if (!status) {
        if (fatal) {
            RTC_LOG(LS_ERROR) << status;
        }
        else {
            RTC_LOG(LS_WARNING) << status;
        }
    }
    return status;
}

} // namespace LiveKitCpp
