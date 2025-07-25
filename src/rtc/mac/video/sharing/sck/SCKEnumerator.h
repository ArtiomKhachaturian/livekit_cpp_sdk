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
#pragma once // SCKEnumerator.h
#include <modules/desktop_capture/desktop_capture_types.h>
#include <string>
#include <vector>

#ifdef __OBJC__
@class NSError;
@class SCWindow;
@class SCDisplay;
@class SCShareableContent;
#else
typedef struct objc_object NSError;
typedef struct objc_object SCWindow;
typedef struct objc_object SCDisplay;
typedef struct objc_object SCShareableContent;
#endif

namespace LiveKitCpp
{

class SCKEnumerator
{
public:
    SCKEnumerator();
    ~SCKEnumerator();
    NSError* updateContent(); // sync operation
    SCWindow* toWindow(const std::string& source) const;
    SCWindow* toWindow(webrtc::WindowId wId) const;
    SCDisplay* toDisplay(const std::string& source) const;
    SCDisplay* toDisplay(webrtc::ScreenId sId) const;
    static std::string windowToString(SCWindow* window);
    static std::string displayToString(SCDisplay* display);
private:
    SCShareableContent* _content = nullptr;
};

} // namespace LiveKitCpp
