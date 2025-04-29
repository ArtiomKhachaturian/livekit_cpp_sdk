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
#include "CGWDescription.h"
#include "RtcUtils.h"
#include "Utils.h"

namespace LiveKitCpp
{

const CFStringRefAutoRelease CGWDescription::_windowServerName = stringToCFString("Window Server");

CGWDescription::CGWDescription(CGWindowID windowId, CFArrayRefAutoRelease description, CFDictionaryRef dictionary)
    : _windowId(windowId)
    , _description(std::move(description))
    , _dictionary(dictionary)
{
}

bool CGWDescription::isWindowServerWindow() const
{
    return isWindowServerWindow(ownerName());
}

std::unique_ptr<CGWDescription> CGWDescription::create(CGWindowID windowId)
{
    std::unique_ptr<CGWDescription> desc;
    if (windowId) {
        const CGWindowID ids[1] = {windowId};
        const CFArrayRefAutoRelease windowIdArray(CFArrayCreate(nullptr, reinterpret_cast<const void**>(&ids), 1, nullptr));
        if (windowIdArray) {
            CFArrayRefAutoRelease windowArray(CGWindowListCreateDescriptionFromArray(windowIdArray));
            if (windowArray && CFArrayGetCount(windowArray) > 0) {
                if (CFDictionaryRef dictionary = (CFDictionaryRef)CFArrayGetValueAtIndex(windowArray, 0)) {
                    desc.reset(new CGWDescription(windowId, std::move(windowArray), dictionary));
                }
            }
        }
    }
    return desc;
}

CGRect CGWDescription::boundsRect() const
{
    if (const auto rBounds = bounds()) {
        CGRect cgRect;
        if (CGRectMakeWithDictionaryRepresentation(rBounds, &cgRect)) {
            return cgRect;
        }
    }
    return CGRectZero;
}

bool CGWDescription::isWindowServerWindow(CGWindowID window)
{
    const auto desc = CGWDescription::create(window);
    return desc && desc->isWindowServerWindow();
}

bool CGWDescription::isWindowServerWindow(CFStringRef ownerName)
{
    if (ownerName) {
        return compareCFStrings(ownerName, _windowServerName, true);
    }
    return false;
}

} // namespace LiveKitCpp
