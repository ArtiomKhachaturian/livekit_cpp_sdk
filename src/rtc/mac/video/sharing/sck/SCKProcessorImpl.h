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
#pragma once // SCKProcessorImpl.h
#include "CapturerState.h"
#include "CFAutoRelease.h"
#include "Listener.h"
#include "SCKFramesReceiver.h"
#include "SCKErrorHandler.h"
#include "VideoFrameBufferPool.h"
#include <atomic>
#include <memory>

#ifdef __OBJC__
@class SCContentFilter;
@class SCWindow;
@class SCDisplay;
@class SCStream;
@class SCStreamConfiguration;
@class SCKStreamOutput;
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

class SCKProcessorImpl : public std::enable_shared_from_this<SCKProcessorImpl>,
                         public SCKFramesReceiver,
                         public SCKErrorHandler
{
public:
    SCKProcessorImpl(bool previewMode, VideoFrameBufferPool framesPool = {});
    ~SCKProcessorImpl() final;
    void setOutputSink(CapturerProxySink* sink) { _sink = sink; }
    bool start();
    bool started() const;
    void stop(bool withHandler = true);
    bool selectDisplay(SCDisplay* display);
    bool selectWindow(SCWindow* window);
    void setExcludedWindow(SCWindow* window);
    void focusOnSelectedWindow();
    void setShowCursor(bool show);
    void setTargetFramerate(int32_t fps);
    void setTargetResolution(int32_t width, int32_t height);
    SCDisplay* selectedDisplay() const;
    SCWindow* selectedWindow() const;
private:
    static CGSize scaleKeepAspectRatio(SCContentFilter* source,
                                       int32_t width, int32_t height);
    static CGSize scaleKeepAspectRatio(SCContentFilter* source, CGSize target);
    static CGSize scaleKeepAspectRatio(const CGSize& source, CGSize target);
    static CGSize pixelsSize(SCContentFilter* scale, CGSize size);
    static CGSize pixelsSize(SCContentFilter* scale);
    bool changeState(CapturerState state);
    bool changeExcludedWindow(SCWindow* window);
    void notifyAboutError(NSError* error, bool fatal = true);
    void setSize(SCContentFilter* filter);
    // destination in pixels
    void setSize(SCContentFilter* filter, CGSize destination);
    bool reconfigureStream(SCContentFilter* filter);
    void updateConfiguration();
    SCContentFilter* createScreenFilter(SCDisplay* display) const;
    SCContentFilter* createWindowFilter(SCWindow* window) const;
    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> fromSampleBuffer(CMSampleBufferRef sampleBuffer) const;
    bool isMyStream(SCStream* stream) const;
    float pointPixelScale() const;
    // impl. of ScreenCaptureFramesReceiver
    void deliverFrame(SCStream* stream, CMSampleBufferRef sampleBuffer) final;
    // impl. of ScreenCaptureErrorHandler
    void processPermanentError(SCStream* stream, NSError* error) final;
private:
    const VideoFrameBufferPool _framesPool;
    SCStreamConfiguration* const _configuration;
    Bricks::SafeObj<CapturerState> _state = CapturerState::Stopped;
    Bricks::Listener<CapturerProxySink*> _sink;
    std::atomic<uint64_t> _targetResolution = 0ULL;
    SCKStreamOutput* _output = nil;
    SCStream* _stream = nil;
    SCContentFilter* _filter = nil;
    SCWindow* _excludedWindow = nil;
};
	
} // namespace LiveKitCpp
