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
#include "SCKScreenCapturer.h"
#include "SCKEnumerator.h"
#include "SCKProcessor.h"
#include "DesktopCapturerUtils.h"
#include "Utils.h"
#import <ScreenCaptureKit/ScreenCaptureKit.h>


namespace LiveKitCpp
{

SCKScreenCapturer::SCKScreenCapturer(webrtc::DesktopCaptureOptions options)
    : MacDesktopCapturer(false, std::move(options))
    , _processor(std::make_unique<SCKProcessor>(screenQueueMaxLen()))
{
    _processor->setOutputSink(this);
}

SCKScreenCapturer::~SCKScreenCapturer()
{
    _processor->stop();
    _processor->setOutputSink(nullptr);
}

bool SCKScreenCapturer::available()
{
    if (@available(macOS 14.0, *)) {
        @autoreleasepool {
            return nil != NSClassFromString(@"SCStream");
        }
    }
    return false;
}

std::string SCKScreenCapturer::selectedSource() const
{
    @autoreleasepool {
        SCDisplay* display = _processor->selectedScreen();
        if (display) {
             return SCKEnumerator::displayToString(display);
        }
    }
    return {};
}

bool SCKScreenCapturer::selectSource(const std::string& source)
{
    if (!source.empty() && enumerate()) {
        @autoreleasepool {
            SCDisplay* display = _enumerator.toScreen(source);
            if (display) {
                return _processor->selectDisplay(display);
            }
        }
    }
    return false;
}

bool SCKScreenCapturer::start()
{
    return _processor->start();
}

bool SCKScreenCapturer::started() const
{
    return _processor->started();
}

void SCKScreenCapturer::stop()
{
    _processor->stop();
}

void SCKScreenCapturer::setPreviewMode(bool preview)
{
    _processor->setShowCursor(!preview);
}

void SCKScreenCapturer::setTargetFramerate(int32_t fps)
{
    _processor->setTargetFramerate(fps);
}

void SCKScreenCapturer::setTargetResolution(int32_t width, int32_t height)
{
    _processor->setTargetResolution(width, height);
}

void SCKScreenCapturer::setExcludedWindow(webrtc::WindowId wId)
{
    if (enumerate()) {
        @autoreleasepool {
            SCWindow* window = _enumerator.toWindow(wId);
            if (window) {
                _processor->setExcludedWindow(window);
            }
        }
    }
}

bool SCKScreenCapturer::enumerate()
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

void SCKScreenCapturer::onCapturingError(std::string details, bool fatal)
{
    notifyAboutError(std::move(details), fatal);
}

void SCKScreenCapturer::OnFrame(const webrtc::VideoFrame& frame)
{
    deliverCaptured(frame);
}

void SCKScreenCapturer::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    processConstraints(c);
}

} // namespace LiveKitCpp
