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
#include "SCKDesktopCapturer.h"
#include "SCKEnumerator.h"
#include "SCKProcessor.h"
#include "DesktopCapturerUtils.h"
#include "DesktopConfiguration.h"
#include "Utils.h"
#import <ScreenCaptureKit/ScreenCaptureKit.h>


namespace LiveKitCpp
{

SCKDesktopCapturer::SCKDesktopCapturer(bool window, bool previewMode,
                                       webrtc::DesktopCaptureOptions options,
                                       VideoFrameBufferPool framesPool)
    : MacDesktopCapturer(window, previewMode, std::move(options), framesPool)
    , _processor(std::make_unique<SCKProcessor>(previewMode, std::move(framesPool)))
{
    _processor->setShowCursor(this->options().prefer_cursor_embedded());
    _processor->setOutputSink(this);
}

SCKDesktopCapturer::~SCKDesktopCapturer()
{
    _processor->stop();
    _processor->setOutputSink(nullptr);
}

bool SCKDesktopCapturer::available()
{
    if (@available(macOS 14.0, *)) {
        @autoreleasepool {
            return nil != NSClassFromString(@"SCStream");
        }
    }
    return false;
}

std::string SCKDesktopCapturer::selectedSource() const
{
    @autoreleasepool {
        if (window()) {
            SCWindow* window = _processor->selectedWindow();
            if (window) {
                 return SCKEnumerator::windowToString(window);
            }
        }
        else {
            SCDisplay* display = _processor->selectedDisplay();
            if (display) {
                 return SCKEnumerator::displayToString(display);
            }
        }
    }
    return {};
}

bool SCKDesktopCapturer::selectSource(const std::string& source)
{
    if (!source.empty() && enumerate()) {
        @autoreleasepool {
            if (window()) {
                SCWindow* window = _enumerator.toWindow(source);
                if (window) {
                    return _processor->selectWindow(window);
                }
            }
            else {
                SCDisplay* display = _enumerator.toDisplay(source);
                if (display) {
                    return _processor->selectDisplay(display);
                }
            }
        }
    }
    return false;
}

bool SCKDesktopCapturer::start()
{
    return _processor->start();
}

bool SCKDesktopCapturer::started() const
{
    return _processor->started();
}

void SCKDesktopCapturer::stop()
{
    _processor->stop();
}

void SCKDesktopCapturer::setTargetFramerate(int32_t fps)
{
    _processor->setTargetFramerate(DesktopConfiguration::boundFramerate(fps));
}

void SCKDesktopCapturer::setTargetResolution(int32_t width, int32_t height)
{
    _processor->setTargetResolution(width, height);
}

void SCKDesktopCapturer::setExcludedWindow(webrtc::WindowId wId)
{
    if (!window() && enumerate()) {
        @autoreleasepool {
            SCWindow* window = _enumerator.toWindow(wId);
            if (window) {
                _processor->setExcludedWindow(window);
            }
        }
    }
}

void SCKDesktopCapturer::focusOnSelectedSource()
{
    if (window() && !previewMode()) {
        _processor->focusOnSelectedWindow();
    }
}

bool SCKDesktopCapturer::enumerate()
{
    @autoreleasepool {
        NSError* error = _enumerator.updateContent();
        if (error) {
            // TODO: replace it to logger output
            notifyAboutError(toString(error), false);
            return false;
        }
    }
    return true;
}

void SCKDesktopCapturer::onCapturingError(std::string details, bool fatal)
{
    notifyAboutError(std::move(details), fatal);
}

void SCKDesktopCapturer::OnFrame(const webrtc::VideoFrame& frame)
{
    deliverCaptured(frame);
}

void SCKDesktopCapturer::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    processConstraints(c);
}

} // namespace LiveKitCpp
