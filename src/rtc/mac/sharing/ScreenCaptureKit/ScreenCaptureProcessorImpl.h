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
#pragma once // ScreenCaptureProcessorImpl.h
#include "CapturerState.h"
#include "CFAutoRelease.h"
#include "Listener.h"
#include "ScreenCaptureFramesReceiver.h"
#include "ScreenCaptureErrorHandler.h"
#include <atomic>
#include <memory>

#ifdef __OBJC__
@class SCContentFilter;
@class SCWindow;
@class SCDisplay;
@class SCStream;
@class SCStreamConfiguration;
@class ScreenCaptureOutput;
#else
typedef struct objc_object SCContentFilter;
typedef struct objc_object SCWindow;
typedef struct objc_object SCDisplay;
typedef struct objc_object SCStream;
typedef struct objc_object SCStreamConfiguration;
typedef struct objc_object ScreenCaptureOutput;
#endif

namespace LiveKitCpp
{

class CapturerProxySink;

class ScreenCaptureProcessorImpl : public std::enable_shared_from_this<ScreenCaptureProcessorImpl>,
                                   public ScreenCaptureFramesReceiver,
                                   public ScreenCaptureErrorHandler
{
public:
    ScreenCaptureProcessorImpl(int queueDepth, OSType pixelFormat);
    ~ScreenCaptureProcessorImpl() final;
    void setOutputSink(CapturerProxySink* sink) { _sink = sink; }
    bool start();
    bool started() const;
    void stop();
    bool selectDisplay(SCDisplay* display);
    bool selectWindow(SCWindow* window);
    void setExcludedWindow(SCWindow* window);
    void setShowCursor(bool show);
    void setTargetFramerate(int32_t fps);
    SCDisplay* selectedScreen() const;
    SCWindow* selectedWindow() const;
private:
    bool changeState(CapturerState state);
    bool changeExcludedWindow(SCWindow* window);
    void notifyAboutError(NSError* error, bool fatal = true);
    bool setSize(size_t width, size_t height);
    bool setSize(const CGSize& size);
    bool setSize(SCWindow* window);
    bool setSize(SCDisplay* display);
    bool reconfigureStream(SCContentFilter* filter);
    void updateConfiguration();
    SCContentFilter* createScreenFilter(SCDisplay* display) const;
    SCContentFilter* createWindowFilter(SCWindow* window) const;
    bool isMyStream(SCStream* stream) const;
    // impl. of ScreenCaptureFramesReceiver
    void deliverFrame(SCStream* stream, CMSampleBufferRef sampleBuffer) final;
    // impl. of ScreenCaptureErrorHandler
    void processPermanentError(SCStream* stream, NSError* error) final;
private:
    SCStreamConfiguration* const _configuration;
    Bricks::SafeObj<CapturerState> _state = CapturerState::Stopped;
    Bricks::Listener<CapturerProxySink*> _sink;
    ScreenCaptureOutput* _output = nil;
    SCStream* _stream = nil;
    NSObject* _selectedObject = nil;
    SCWindow* _excludedWindow = nil;
};
	
} // namespace LiveKitCpp
