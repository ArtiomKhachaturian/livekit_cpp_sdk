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
#pragma once // VTSession.h
#include "CFNumber.h"
#include "RtcUtils.h"
#include "VideoUtils.h"
#include <memory>

namespace LiveKitCpp
{

template <typename TFormatType, typename VTSessionType>
class VTSession
{
public:
    const auto& format() const noexcept { return _format; }
    VTSessionType CM_NONNULL sessionRef() const { return _sessionRef; }
    bool hardwareAccelerated() const { return _hardwareAccelerated; }
    operator VTSessionType CM_NONNULL() const { return sessionRef(); }
    // diff between input & output frames
    virtual uint64_t pendingFramesCount() const = 0;
    // return the last status of output processing
    virtual webrtc::RTCError lastOutputStatus() const = 0;
    // Convenience functions for setting a VT properties.
    template <typename TNumberType>
    webrtc::RTCError setProperty(CFStringRef CM_NONNULL key, TNumberType value);
    webrtc::RTCError setProperty(CFStringRef CM_NONNULL key, bool value);
    webrtc::RTCError setProperty(CFStringRef CM_NONNULL key, CFStringRef CM_NULLABLE value);
    webrtc::RTCError setProperty(CFStringRef CM_NONNULL key, CFDictionaryRef CM_NULLABLE value);
    webrtc::RTCError setProperty(CFStringRef CM_NONNULL key, CFArrayRef CM_NULLABLE value);
    webrtc::RTCError propertySupported(CFStringRef CM_NONNULL key) const;
protected:
    VTSession(TFormatType format, VTSessionType sessionRef, bool hardwareAccelerated);
    virtual ~VTSession() = default;
private:
    const TFormatType _format;
    const CFAutoRelease<VTSessionType> _sessionRef;
    const bool _hardwareAccelerated;
    CFDictionaryRefAutoRelease _supportedProperties;
};

template <typename TFormatType, typename VTSessionType>
inline VTSession<TFormatType, VTSessionType>::VTSession(TFormatType format,
                                                        VTSessionType sessionRef,
                                                        bool hardwareAccelerated)
    : _format(std::move(format))
    , _sessionRef(sessionRef)
    , _hardwareAccelerated(hardwareAccelerated)
{
}

template <typename TFormatType, typename VTSessionType>
template <typename TNumberType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           TNumberType value)
{
    return toRtcError(VTSessionSetProperty(sessionRef(), key, createCFNumber(value)));
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           bool value)
{
    return toRtcError(VTSessionSetProperty(sessionRef(), key, value ? kCFBooleanTrue : kCFBooleanFalse));
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFStringRef CM_NULLABLE value)
{
    return toRtcError(VTSessionSetProperty(sessionRef(), key, value));
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFDictionaryRef CM_NULLABLE value)
{
    return toRtcError(VTSessionSetProperty(sessionRef(), key, value));
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFArrayRef CM_NULLABLE value)
{
    return VTSessionSetProperty(sessionRef(), key, value);
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::propertySupported(CFStringRef CM_NONNULL key) const
{
    if (!_supportedProperties) {
        CFDictionaryRef* out = _supportedProperties.pointer();
        const auto status = VTSessionCopySupportedPropertyDictionary(sessionRef(), out);
        if (noErr != status) {
            return toRtcError(status);
        }
    }
    if (CFDictionaryContainsKey(_supportedProperties, key)) {
        return {};
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_RANGE);
}

} // namespace LiveKitCpp
