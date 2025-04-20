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
#pragma once
#include "CFAutoRelease.h"
#include <CoreGraphics/CGWindow.h>
#include <CoreFoundation/CFNumber.h>
#include <memory>

namespace LiveKitCpp
{

class CGWDescription
{
public:
    ~CGWDescription() = default;
    CGWindowID windowId() const { return _windowId; }
    CFDictionaryRef dictionary() const { return _dictionary; }
    CFNumberRef alpha() const { return value<CFNumberRef>(kCGWindowAlpha); }
    CFBooleanRef backingLocationVideoMemory() const { return value<CFBooleanRef>(kCGWindowBackingLocationVideoMemory); }
    CFDictionaryRef bounds() const { return value<CFDictionaryRef>(kCGWindowBounds); }
    CGRect boundsRect() const;
    CFBooleanRef onScreen() const { return value<CFBooleanRef>(kCGWindowIsOnscreen); }
    CFNumberRef layer() const { return value<CFNumberRef>(kCGWindowLayer); }
    CFNumberRef memoryUsage() const { return value<CFNumberRef>(kCGWindowMemoryUsage); }
    CFStringRef name() const { return value<CFStringRef>(kCGWindowName); }
    CFNumberRef number() const { return value<CFNumberRef>(kCGWindowNumber); }
    CFStringRef ownerName() const { return value<CFStringRef>(kCGWindowOwnerName); }
    CFNumberRef ownerPID() const { return value<CFNumberRef>(kCGWindowOwnerPID); }
    CFNumberRef sharingState() const { return value<CFNumberRef>(kCGWindowSharingState); }
    CFNumberRef storeType() const { return value<CFNumberRef>(kCGWindowStoreType); }
    bool isWindowServerWindow() const;
    static std::unique_ptr<CGWDescription> create(CGWindowID windowId);
    static bool isWindowServerWindow(CGWindowID window);
    static bool isWindowServerWindow(CFStringRef ownerName);
protected:
    CGWDescription(CGWindowID windowId, CFArrayRefAutoRelease description, CFDictionaryRef dictionary);
private:
    template<typename TValue>
    TValue value(const void* key) const { return reinterpret_cast<TValue>(CFDictionaryGetValue(dictionary(), key)); }
private:
    static const CFStringRefAutoRelease _windowServerName;
    const CGWindowID _windowId;
    const CFArrayRefAutoRelease _description;
    const CFDictionaryRef _dictionary;
};

} // namespace LiveKitCpp
