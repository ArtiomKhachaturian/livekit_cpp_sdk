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
#include <CoreFoundation/CFDictionary.h>
#include <cstddef>

namespace LiveKitCpp
{

template <typename TCFRef, typename Traits = CFAutoReleaseTraits<TCFRef>>
class CFAutoRelease
{
public:
    CFAutoRelease()
        : _ref(Traits::invalidValue())
    {
    }
    CFAutoRelease(std::nullptr_t)
        : _ref(Traits::invalidValue())
    {
    }
    CFAutoRelease(TCFRef ref, bool retain = false)
        : _ref(Traits::invalidValue())
    {
        set(ref, retain);
    }
    CFAutoRelease(CFAutoRelease&& tmp)
        : _ref(Traits::invalidValue())
    {
        set(tmp._ref, false);
        tmp._ref = Traits::invalidValue();
    }
    CFAutoRelease(const CFAutoRelease& other)
        : CFAutoRelease(other._ref, true)
    {
    }
    // This allows passing an object to a function that takes its superclass.
    template <typename RCFRef, typename RTraits>
    explicit CFAutoRelease(const CFAutoRelease<RCFRef, RTraits>& thatAsSubclass)
        : _ref(Traits::invalidValue())
    {
        if (RTraits::invalidValue() != thatAsSubclass.ref()) {
            set(thatAsSubclass.ref(), true);
        }
    }
    ~CFAutoRelease() { release(); }
    TCFRef ref(bool retain = false) const
    {
        if (retain && Traits::invalidValue() != _ref) {
            return Traits::retain(_ref);
        }
        return _ref;
    }
    TCFRef* pointer() { return &_ref; }
    operator TCFRef() const { return ref(); }
    operator bool() const { return valid(); }
    CFAutoRelease& operator=(const CFAutoRelease& other)
    {
        if (&other != this) {
            set(other._ref, true);
        }
        return *this;
    }
    CFAutoRelease& operator=(CFAutoRelease&& tmp)
    {
        if (&tmp != this) {
            set(tmp._ref, false);
            tmp._ref = Traits::invalidValue();
        }
        return *this;
    }

    void set(TCFRef ref, bool retain)
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

    void release()
    {
        if (Traits::invalidValue() != _ref) {
            Traits::release(_ref);
            _ref = Traits::invalidValue();
        }
    }
    
    void retain()
    {
        if (Traits::invalidValue() != _ref) {
            _ref = Traits::retain(_ref);
        }
    }

    bool valid() const { return Traits::invalidValue() != ref(); }

private:
    TCFRef _ref;
};

using CFDictionaryRefAutoRelease = CFAutoRelease<CFDictionaryRef>;
using CFMutableDictionaryRefAutoRelease = CFAutoRelease<CFMutableDictionaryRef>;
using CFArrayRefAutoRelease = CFAutoRelease<CFArrayRef>;
using CFStringRefAutoRelease = CFAutoRelease<CFStringRef>;


} // namespace LiveKitCpp
