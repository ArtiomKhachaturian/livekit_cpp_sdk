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
#include "CompletionStatusOr.h"
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
    bool hardwareAccelerated() const noexcept { return _hardwareAccelerated; }
    operator VTSessionType CM_NONNULL() const { return sessionRef(); }
    explicit operator bool() const { return valid(); }
    // diff between input & output frames
    virtual uint64_t pendingFramesCount() const = 0;
    // return the last status of output processing
    virtual CompletionStatus lastOutputStatus() const = 0;
    // Convenience functions for setting a VT properties.
    template <typename TNumberType>
    CompletionStatus setProperty(CFStringRef CM_NONNULL key, TNumberType value);
    CompletionStatus setProperty(CFStringRef CM_NONNULL key, bool value);
    CompletionStatus setProperty(CFStringRef CM_NONNULL key, CFStringRef CM_NULLABLE value);
    CompletionStatus setProperty(CFStringRef CM_NONNULL key, CFDictionaryRef CM_NULLABLE value);
    CompletionStatus setProperty(CFStringRef CM_NONNULL key, CFArrayRef CM_NULLABLE value);
    CompletionStatus propertySupported(CFStringRef CM_NONNULL key) const;
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
inline CompletionStatus VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           TNumberType value)
{
    if (const auto session = sessionRef()) {
        return COMPLETION_STATUS(VTSessionSetProperty(session, key, createCFNumber(value)));
    }
    return COMPLETION_STATUS_INVALID_STATE;
}

template <typename TFormatType, typename VTSessionType>
inline CompletionStatus VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           bool value)
{
    if (const auto session = sessionRef()) {
        return COMPLETION_STATUS(VTSessionSetProperty(session, key, value ? kCFBooleanTrue : kCFBooleanFalse));
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

template <typename TFormatType, typename VTSessionType>
inline CompletionStatus VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFStringRef CM_NULLABLE value)
{
    if (const auto session = sessionRef()) {
        return COMPLETION_STATUS(VTSessionSetProperty(session, key, value));
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

template <typename TFormatType, typename VTSessionType>
inline CompletionStatus VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFDictionaryRef CM_NULLABLE value)
{
    if (const auto session = sessionRef()) {
        return COMPLETION_STATUS(VTSessionSetProperty(session, key, value));
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

template <typename TFormatType, typename VTSessionType>
inline CompletionStatus VTSession<TFormatType, VTSessionType>::setProperty(CFStringRef CM_NONNULL key,
                                                                           CFArrayRef CM_NULLABLE value)
{
    if (const auto session = sessionRef()) {
        return COMPLETION_STATUS(VTSessionSetProperty(session, key, value));
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

template <typename TFormatType, typename VTSessionType>
inline CompletionStatus VTSession<TFormatType, VTSessionType>::propertySupported(CFStringRef CM_NONNULL key) const
{
    if (const auto session = sessionRef()) {
        if (!_supportedProperties) {
            CFDictionaryRef* out = _supportedProperties.pointer();
            auto status = COMPLETION_STATUS(VTSessionCopySupportedPropertyDictionary(session, out));
            if (!status) {
                return status;
            }
        }
        if (CFDictionaryContainsKey(_supportedProperties, key)) {
            return {};
        }
        return COMPLETION_STATUS(dcmNoRecordErr);
    }
    return COMPLETION_STATUS(kVTInvalidSessionErr);
}

} // namespace LiveKitCpp
