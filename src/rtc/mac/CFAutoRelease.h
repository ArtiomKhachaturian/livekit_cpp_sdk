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
#pragma once // CFAutoRelease.h
#include "CFAutoReleaseTraits.h"
#include <CoreFoundation/CFArray.h>
#include <CoreFoundation/CFData.h>
#include <CoreFoundation/CFDictionary.h>
#include <cstddef>

namespace LiveKitCpp
{

template <typename TCFRef, typename Traits = CFAutoReleaseTraits<TCFRef>>
class CFAutoRelease
{
public:
    CFAutoRelease();
    CFAutoRelease(std::nullptr_t);
    CFAutoRelease(TCFRef ref, bool retain = false);
    CFAutoRelease(CFAutoRelease&& tmp);
    CFAutoRelease(const CFAutoRelease& other);
    // This allows passing an object to a function that takes its superclass.
    template <typename RCFRef, typename RTraits>
    explicit CFAutoRelease(const CFAutoRelease<RCFRef, RTraits>& thatAsSubclass);
    ~CFAutoRelease() { release(); }
    TCFRef ref(bool retain = false) const;
    TCFRef* pointer() { return &_ref; }
    operator TCFRef() const { return ref(); }
    explicit operator bool() const { return valid(); }
    CFAutoRelease& operator=(const CFAutoRelease& other);
    CFAutoRelease& operator=(CFAutoRelease&& tmp);
    void set(TCFRef ref, bool retain);
    void release();
    void retain();
    bool valid() const { return Traits::invalidValue() != ref(); }
private:
    TCFRef _ref;
};

using CFDictionaryRefAutoRelease = CFAutoRelease<CFDictionaryRef>;
using CFMutableDictionaryRefAutoRelease = CFAutoRelease<CFMutableDictionaryRef>;
using CFArrayRefAutoRelease = CFAutoRelease<CFArrayRef>;
using CFStringRefAutoRelease = CFAutoRelease<CFStringRef>;
using CFDataRefAutoRelease = CFAutoRelease<CFDataRef>;


template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>::CFAutoRelease()
    : _ref(Traits::invalidValue())
{
}

template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>::CFAutoRelease(std::nullptr_t)
    : _ref(Traits::invalidValue())
{
}

template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>::CFAutoRelease(TCFRef ref, bool retain)
    : _ref(Traits::invalidValue())
{
    set(ref, retain);
}

template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>::CFAutoRelease(CFAutoRelease&& tmp)
    : _ref(Traits::invalidValue())
{
    set(tmp._ref, false);
    tmp._ref = Traits::invalidValue();
}

template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>::CFAutoRelease(const CFAutoRelease& other)
    : CFAutoRelease(other._ref, true)
{
}

template <typename TCFRef, typename Traits>
template <typename RCFRef, typename RTraits>
inline CFAutoRelease<TCFRef,Traits>::CFAutoRelease(const CFAutoRelease<RCFRef,
                                                   RTraits>& thatAsSubclass)
    : _ref(Traits::invalidValue())
{
    if (RTraits::invalidValue() != thatAsSubclass.ref()) {
        set(thatAsSubclass.ref(), true);
    }
}

template <typename TCFRef, typename Traits>
inline TCFRef CFAutoRelease<TCFRef,Traits>::ref(bool retain ) const
{
    if (retain && Traits::invalidValue() != _ref) {
        return Traits::retain(_ref);
    }
    return _ref;
}

template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>& CFAutoRelease<TCFRef,Traits>::
    operator=(const CFAutoRelease& other)
{
    if (&other != this) {
        set(other._ref, true);
    }
    return *this;
}

template <typename TCFRef, typename Traits>
inline CFAutoRelease<TCFRef,Traits>& CFAutoRelease<TCFRef,Traits>::
    operator=(CFAutoRelease&& tmp)
{
    if (&tmp != this) {
        set(tmp._ref, false);
        tmp._ref = Traits::invalidValue();
    }
    return *this;
}

template <typename TCFRef, typename Traits>
inline void CFAutoRelease<TCFRef,Traits>::set(TCFRef ref, bool retain)
{
    if (ref != _ref) {
        release();
        if (retain && Traits::invalidValue() != ref) {
            _ref = Traits::retain(ref);
        } else {
            _ref = ref;
        }
    }
}

template <typename TCFRef, typename Traits>
inline void CFAutoRelease<TCFRef,Traits>::release()
{
    if (Traits::invalidValue() != _ref) {
        Traits::release(_ref);
        _ref = Traits::invalidValue();
    }
}

template <typename TCFRef, typename Traits>
inline void CFAutoRelease<TCFRef,Traits>::retain()
{
    if (Traits::invalidValue() != _ref) {
        _ref = Traits::retain(_ref);
    }
}

} // namespace LiveKitCpp
