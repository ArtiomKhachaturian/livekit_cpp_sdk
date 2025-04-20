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
#include "SCKProcessorImpl.h"
#include "SCKDelegate.h"
#include "SCKStreamOutput.h"
#include "CapturerProxySink.h"
#include "CoreVideoPixelBuffer.h"
#include "Utils.h"
#include "VideoUtils.h"
#include <rtc_base/time_utils.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <iostream>

namespace {

inline NSArray<SCWindow*>* excludingWindow(SCWindow* window = nil) {
    NSMutableArray<SCWindow*>* excluded = [NSMutableArray<SCWindow*> new];
    if (window) {
        [excluded addObject:window];
    }
    return excluded;
}

template <typename T>
inline T roundCGFloat(CGFloat f) {
    return static_cast<int32_t>(std::round(f));
}

}

namespace LiveKitCpp
{

SCKProcessorImpl::SCKProcessorImpl(int queueDepth, OSType pixelFormat)
    : _configuration([SCStreamConfiguration new])
{
    _configuration.queueDepth = queueDepth;
    _configuration.pixelFormat = pixelFormat;
    _configuration.colorSpaceName = kCGColorSpaceSRGB;
    _configuration.colorMatrix = kCGDisplayStreamYCbCrMatrix_SMPTE_240M_1995;
    //_configuration.captureResolution = SCCaptureResolutionNominal;
}

SCKProcessorImpl::~SCKProcessorImpl()
{
    _output = nil;
    _stream = nil;
    _selectedObject = nil;
    _excludedWindow = nil;
}

bool SCKProcessorImpl::start()
{
    @synchronized (_configuration) {
        if (_stream && changeState(CapturerState::Starting)) {
            auto weakSelf = weak_from_this();
            [_stream startCaptureWithCompletionHandler:^(NSError * _Nullable error) {
                if (const auto strongSelf = weakSelf.lock()) {
                    if (!error) {
                        strongSelf->changeState(CapturerState::Started);
                    }
                    else {
                        strongSelf->notifyAboutError(error);
                    }
                }
            }];
            return true;
        }
    }
    return false;
}

bool SCKProcessorImpl::started() const
{
    switch (_state()) {
        case CapturerState::Starting:
        case CapturerState::Started:
            return true;
        default:
            break;
    }
    return false;
}

void SCKProcessorImpl::stop()
{
    @synchronized (_configuration) {
        if (_stream && changeState(CapturerState::Stopping)) {
            auto weakSelf = weak_from_this();
            [_stream stopCaptureWithCompletionHandler:^(NSError * _Nullable error) {
                if (const auto strongSelf = weakSelf.lock()) {
                    if (error && error.code != SCStreamErrorAttemptToStopStreamState) {
                        strongSelf->notifyAboutError(error, false);
                    }
                    else {
                        strongSelf->changeState(CapturerState::Stopped);
                    }
                }
            }];
        }
    }
}

bool SCKProcessorImpl::selectDisplay(SCDisplay* display)
{
    if (display) {
        stop();
        @autoreleasepool {
            SCContentFilter* filter = createScreenFilter(display);
            if (filter && setSize(filter) && reconfigureStream(filter)) {
                @synchronized (_configuration) {
                    _selectedObject = display;
                }
                return true;
            }
        }
    }
    return false;
}

bool SCKProcessorImpl::selectWindow(SCWindow* window)
{
    if (window) {
        stop();
        @autoreleasepool {
            SCContentFilter* filter = createWindowFilter(window);
            if (filter && setSize(filter) && reconfigureStream(filter)) {
                @synchronized (_configuration) {
                    _selectedObject = window;
                }
                return true;
            }
        }
    }
    return false;
}

void SCKProcessorImpl::setExcludedWindow(SCWindow* window)
{
    if (changeExcludedWindow(window)) {
        @autoreleasepool {
            SCContentFilter* filter = createScreenFilter(selectedScreen());
            if (filter) {
                @synchronized (_configuration) {
                    if (_stream) {
                        auto weakSelf = weak_from_this();
                        [_stream updateContentFilter:filter completionHandler:^(NSError * _Nullable error) {
                            if (const auto strongSelf = weakSelf.lock()) {
                                strongSelf->notifyAboutError(error, false);
                            }
                        }];
                    }
                }
            }
        }
    }
}

void SCKProcessorImpl::setShowCursor(bool show)
{
    const BOOL val = show ? YES : NO;
    if (val != _configuration.showsCursor) {
        _configuration.showsCursor = val;
        updateConfiguration();
    }
}

void SCKProcessorImpl::setTargetFramerate(int32_t fps)
{
    if (fps > 0) {
        const auto interval = CMTimeMake(uint64_t(std::round(1000ULL / fps)), rtc::kNumMillisecsPerSec);
        if (0 != CMTimeCompare(interval, _configuration.minimumFrameInterval)) {
            _configuration.minimumFrameInterval = interval;
            updateConfiguration();
        }
    }
}

void SCKProcessorImpl::setTargetResolution(int32_t width, int32_t height)
{
    if (width > 1 && height > 1) {
        width = roundCGFloat<int32_t>(width / _lastPointPixelScale);
        height = roundCGFloat<int32_t>(height / _lastPointPixelScale);
        if (setSize(width, height, _lastPointPixelScale)) {
            _targetResolution = clueToUint64(width, height);
        }
    }
    else {
        _targetResolution = 0ULL;
        @autoreleasepool {
            if (SCDisplay* display = selectedScreen()) {
                setSize(display, _lastPointPixelScale);
            }
            else if (SCWindow* window = selectedWindow()) {
                setSize(window, _lastPointPixelScale);
            }
        }
    }
}

SCDisplay* SCKProcessorImpl::selectedScreen() const
{
    @synchronized (_configuration) {
        if (_selectedObject && [_selectedObject isKindOfClass:[SCDisplay class]]) {
            return (SCDisplay*)_selectedObject;
        }
    }
    return nil;
}

SCWindow* SCKProcessorImpl::selectedWindow() const
{
    @synchronized (_configuration) {
        if (_selectedObject && [_selectedObject isKindOfClass:[SCWindow class]]) {
            return (SCWindow*)_selectedObject;
        }
    }
    return nil;
}

bool SCKProcessorImpl::changeState(CapturerState state)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_state);
        if (acceptState(_state, state)) {
            _state = state;
            changed = true;
        }
    }
    if (changed) {
        _sink.invoke(&CapturerProxySink::onStateChanged, state);
    }
    return changed;
}

bool SCKProcessorImpl::changeExcludedWindow(SCWindow* window)
{
    @synchronized (_configuration) {
        if (window != _excludedWindow) {
            _excludedWindow = window;
            return true;
        }
    }
    return false;
}

void SCKProcessorImpl::notifyAboutError(NSError* error, bool fatal)
{
    if (error) {
        if (fatal) {
            changeState(CapturerState::Stopped);
        }
        _sink.invoke(&CapturerProxySink::onCapturingError, toString(error), fatal);
    }
}

bool SCKProcessorImpl::setSize(size_t width, size_t height, float pointPixelScale)
{
    if (width > 0 && height > 0) {
        width = roundCGFloat<size_t>(width * pointPixelScale);
        height = roundCGFloat<size_t>(height * pointPixelScale);
        _lastPointPixelScale = pointPixelScale;
        if (width != _configuration.width || height != _configuration.height) {
            _configuration.width = width;
            _configuration.height = height;
            updateConfiguration();
        }
        return true;
    }
    return false;
}

bool SCKProcessorImpl::setSize(const CGSize& size, float pointPixelScale)
{
    return setSize(roundCGFloat<size_t>(size.width), roundCGFloat<size_t>(size.height), pointPixelScale);
}

bool SCKProcessorImpl::setSize(SCWindow* window, float pointPixelScale)
{
    return window && setSize(window.frame.size, pointPixelScale);
}

bool SCKProcessorImpl::setSize(SCDisplay* display, float pointPixelScale)
{
    return display && setSize(display.frame.size, pointPixelScale);
}

bool SCKProcessorImpl::setSize(SCContentFilter* filter)
{
    if (filter) {
        if (const auto targetResolution = _targetResolution.load()) {
            return setSize(extractHiWord(targetResolution),
                           extractLoWord(targetResolution),
                           filter.pointPixelScale);
        }
        return setSize(filter.contentRect.size, filter.pointPixelScale);
    }
    return false;
}

bool SCKProcessorImpl::reconfigureStream(SCContentFilter* filter)
{
    bool reconfigured = false;
    if (filter) {
        @autoreleasepool {
            auto delegate = [[SCKDelegate alloc] initWith:weak_from_this()];
            SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                                  configuration:_configuration
                                                       delegate:delegate];
            if (stream) {
                NSError* error = nil;
                SCKStreamOutput* output = [[SCKStreamOutput alloc] initWith:weak_from_this()];
                if ([stream addStreamOutput:output
                                       type:SCStreamOutputTypeScreen
                         sampleHandlerQueue:nil
                                      error:&error]) {
                    @synchronized (_configuration) {
                        _output = output;
                        _stream = stream;
                    }
                    reconfigured = true;
                }
                else {
                    notifyAboutError(error);
                }
            }
        }
    }
    return reconfigured;
}

void SCKProcessorImpl::updateConfiguration()
{
    @synchronized (_configuration) {
        if (_stream) {
            auto weakSelf = weak_from_this();
            [_stream updateConfiguration:_configuration
                       completionHandler:^(NSError * _Nullable error) {
                if (const auto strongSelf = weakSelf.lock()) {
                    strongSelf->notifyAboutError(error, false);
                }
            }];
        }
    }
}

SCContentFilter* SCKProcessorImpl::createScreenFilter(SCDisplay* display) const
{
    if (display) {
        @synchronized (_configuration) {
            return [[SCContentFilter alloc] initWithDisplay:display
                                           excludingWindows:excludingWindow(_excludedWindow)];
        }
    }
    return nil;
}

SCContentFilter* SCKProcessorImpl::createWindowFilter(SCWindow* window) const
{
    if (window) {
        return [[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
    }
    return nil;
}

bool SCKProcessorImpl::isMyStream(SCStream* stream) const
{
    @synchronized (_configuration) {
        return _stream == stream;
    }
}

void SCKProcessorImpl::deliverFrame(SCStream* stream, CMSampleBufferRef sampleBuffer)
{
    if (isMyStream(stream)) {
        if (const auto frame = createVideoFrame(CoreVideoPixelBuffer::createFromSampleBuffer(sampleBuffer))) {
            _sink.invoke(&CapturerProxySink::OnFrame, frame.value());
        }
        else {
            _sink.invoke(&CapturerProxySink::OnDiscardedFrame);
        }
    }
}

void SCKProcessorImpl::processPermanentError(SCStream* stream, NSError* error)
{
    if (isMyStream(stream)) {
        notifyAboutError(error);
    }
}

} // namespace LiveKitCpp
