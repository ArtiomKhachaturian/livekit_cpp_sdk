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
    bool valid() const { return _sessionRef.valid(); }
    const auto& format() const noexcept { return _format; }
    VTSessionType CM_NONNULL sessionRef() const { return _sessionRef; }
    bool hardwareAccelerated() const { return _hardwareAccelerated; }
    operator VTSessionType CM_NONNULL() const { return sessionRef(); }
    explicit operator bool() const { return valid(); }
    // diff between input & output frames
    virtual uint64_t pendingFramesCount() const = 0;
    // return the last status of output processing
    virtual OSStatus lastOutputStatus() const = 0;
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
    VTSession() = default;
    VTSession(const VTSession&) = delete;
    VTSession(VTSession&&) = default;
    virtual ~VTSession() = default;
    VTSession& operator = (const VTSession&) = delete;
    VTSession& operator = (VTSession&&) = default;
private:
    TFormatType _format = {};
    CFAutoRelease<VTSessionType> _sessionRef;
    bool _hardwareAccelerated = false;
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
    if (const auto session = sessionRef()) {
        return toRtcError(VTSessionSetProperty(session, key, createCFNumber(value)));
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_STATE);
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           bool value)
{
    if (const auto session = sessionRef()) {
        return toRtcError(VTSessionSetProperty(session, key, value ? kCFBooleanTrue : kCFBooleanFalse));
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_STATE);
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFStringRef CM_NULLABLE value)
{
    if (const auto session = sessionRef()) {
        return toRtcError(VTSessionSetProperty(session, key, value));
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_STATE);
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFDictionaryRef CM_NULLABLE value)
{
    if (const auto session = sessionRef()) {
        return toRtcError(VTSessionSetProperty(session, key, value));
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_STATE);
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFArrayRef CM_NULLABLE value)
{
    if (const auto session = sessionRef()) {
        return toRtcError(VTSessionSetProperty(session, key, value));
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_STATE);
}

template <typename TFormatType, typename VTSessionType>
inline webrtc::RTCError VTSession<TFormatType, VTSessionType>::propertySupported(CFStringRef CM_NONNULL key) const
{
    if (const auto session = sessionRef()) {
        if (!_supportedProperties) {
            CFDictionaryRef* out = _supportedProperties.pointer();
            const auto status = VTSessionCopySupportedPropertyDictionary(session, out);
            if (noErr != status) {
                return toRtcError(status);
            }
        }
        if (CFDictionaryContainsKey(_supportedProperties, key)) {
            return {};
        }
        return webrtc::RTCError(webrtc::RTCErrorType::INVALID_RANGE);
    }
    return webrtc::RTCError(webrtc::RTCErrorType::INVALID_STATE);
}

} // namespace LiveKitCpp
