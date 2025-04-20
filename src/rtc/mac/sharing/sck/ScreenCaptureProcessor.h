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
#pragma once // ScreenCaptureProcessor.h
#include "RtcObject.h"
#include <CoreVideo/CoreVideo.h>

#ifdef __OBJC__
@class SCWindow;
@class SCDisplay;
#else
typedef struct objc_object SCWindow;
typedef struct objc_object SCDisplay;
#endif

namespace LiveKitCpp
{

class ScreenCaptureProcessorImpl;
class CapturerProxySink;

class ScreenCaptureProcessor : public RtcObject<ScreenCaptureProcessorImpl>
{
public:
    ScreenCaptureProcessor(int queueDepth, OSType pixelFormat);
    ~ScreenCaptureProcessor();
    void setOutputSink(CapturerProxySink* sink);
    bool start();
    bool started() const;
    void stop();
    bool selectDisplay(SCDisplay* display);
    bool selectWindow(SCWindow* window);
    void setExcludedWindow(SCWindow* window);
    void setShowCursor(bool show);
    void setTargetFramerate(int32_t fps);
    void setTargetResolution(int32_t width, int32_t height);
    SCDisplay* selectedScreen() const;
    SCWindow* selectedWindow() const;
};

} // namespace LiveKitCpp
