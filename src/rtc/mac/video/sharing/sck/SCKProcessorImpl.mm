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
#include "IOSurfaceBuffer.h"
#include "Utils.h"
#include "VideoUtils.h"
#include <rtc_base/time_utils.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

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

inline CGRect scaleRect(CGRect input, float scale) {
    input.origin.x *= scale;
    input.origin.y *= scale;
    input.size.width *= scale;
    input.size.height *= scale;
    return input;
}

inline CGSize scaleWithAspectRatio(const CGSize& originalSize,
                                   const CGSize& targetSize) {
    const auto sfw = targetSize.width / originalSize.width;
    const auto sfh = targetSize.height / originalSize.height;
    const auto sf  = fminf(sfw, sfh);
    return CGSizeMake(originalSize.width * sf, originalSize.height * sf);
}

}

namespace LiveKitCpp
{

SCKProcessorImpl::SCKProcessorImpl(VideoFrameBufferPool framesPool)
    : _framesPool(std::move(framesPool))
    , _configuration([SCStreamConfiguration new])
{
    // the same value as in WebKit, system default is 3, max should not exceed 8 frames
    _configuration.queueDepth = 6;
    _configuration.colorSpaceName = kCGColorSpaceSRGB;
    _configuration.colorMatrix = kCGDisplayStreamYCbCrMatrix_SMPTE_240M_1995;
    _configuration.pixelFormat = formatBGRA32();
    _configuration.captureResolution = SCCaptureResolutionAutomatic;
}

SCKProcessorImpl::~SCKProcessorImpl()
{
    _output = nil;
    _stream = nil;
    _filter = nil;
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
            if (filter) {
                setSize(filter);
                return reconfigureStream(filter);
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
            if (filter) {
                setSize(filter);
                return reconfigureStream(filter);
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
        const auto interval = CMTimeMake(1, fps);
        if (0 != CMTimeCompare(interval, _configuration.minimumFrameInterval)) {
            _configuration.minimumFrameInterval = interval;
            updateConfiguration();
        }
    }
}

void SCKProcessorImpl::setTargetResolution(int32_t width, int32_t height)
{
    width = std::max<int32_t>(0, width);
    height = std::max<int32_t>(0, height);
    const auto resolution = clueToUint64(width, height);
    if (exchangeVal(resolution, _targetResolution)) {
        @synchronized (_configuration) {
            if (_filter) {
                if (width > 0 && height > 0) {
                    setSize(_filter, scaledRect(_filter, width, height));
                }
                else {
                    setSize(_filter);
                }
            }
        }
    }
}

SCDisplay* SCKProcessorImpl::selectedScreen() const
{
    @synchronized (_configuration) {
        if (_filter) {
            NSArray<SCDisplay*>* displays = _filter.includedDisplays;
            if (displays && displays.count) {
                return displays.firstObject;
            }
        }
    }
    return nil;
}

SCWindow* SCKProcessorImpl::selectedWindow() const
{
    @synchronized (_configuration) {
        if (_filter) {
            NSArray<SCWindow*>* windows = _filter.includedWindows;
            if (windows && windows.count) {
                return windows.firstObject;
            }
        }
    }
    return nil;
}

CGRect SCKProcessorImpl::scaledRect(SCContentFilter* filter,
                                    int32_t width, int32_t height)
{
    if (filter) {
        auto size = filter.contentRect.size;
        size = scaleWithAspectRatio(size, CGSizeMake(width, height));
        return CGRectMake(0.f, 0.f, size.width, size.height);
    }
    return CGRectZero;
}

bool SCKProcessorImpl::changeState(CapturerState state)
{
    bool changed = false;
    {
        LOCK_WRITE_SAFE_OBJ(_state);
        if (_state != state && acceptState(_state, state)) {
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

void SCKProcessorImpl::setSize(SCContentFilter* filter)
{
    if (filter) {
        if (const auto targetResolution = _targetResolution.load()) {
            const int32_t w = extractHiWord(targetResolution);
            const int32_t h = extractLoWord(targetResolution);
            setSize(filter, scaledRect(filter, w, h));
        }
        else {
            setSize(filter, scaleRect(filter.contentRect, filter.pointPixelScale));
        }
    }
}

void SCKProcessorImpl::setSize(SCContentFilter* filter, CGRect destination)
{
    if (filter && !CGRectIsEmpty(destination)) {
        const float pointPixelScale = filter.pointPixelScale;
        const CGRect source = filter.contentRect;
        const auto width = roundCGFloat<size_t>(destination.size.width);
        const auto height = roundCGFloat<size_t>(destination.size.height);
        if (_configuration.width != width || _configuration.height != height ||
            !CGRectEqualToRect(source, _configuration.sourceRect) ||
            !CGRectEqualToRect(destination, _configuration.destinationRect)) {
            _configuration.sourceRect = source;
            _configuration.destinationRect = destination;
            _configuration.width = width;
            _configuration.height = height;
            updateConfiguration();
        }
    }
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
                        _filter = filter;
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
        auto buffer = CoreVideoPixelBuffer::createFromSampleBuffer(sampleBuffer, _framesPool);
        if (!buffer) {
            buffer = IOSurfaceBuffer::createFromSampleBuffer(sampleBuffer, _framesPool);
        }
        if (const auto frame = createVideoFrame(buffer)) {
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
