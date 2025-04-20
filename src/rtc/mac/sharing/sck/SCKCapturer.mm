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
#include "SCKCapturer.h"
#include "SCKEnumerator.h"
#include "SCKProcessor.h"
#include "CoreVideoPixelBuffer.h"
#include "DesktopCapturerUtils.h"
#include "Utils.h"
#import <ScreenCaptureKit/ScreenCaptureKit.h>

namespace  {

using namespace LiveKitCpp;

inline OSType videoFormat(bool window) {
    return window ? CoreVideoPixelBuffer::formatBGRA32() : CoreVideoPixelBuffer::formatNV12Video();
}

}

namespace LiveKitCpp
{

SCKCapturer::SCKCapturer(bool window, webrtc::DesktopCaptureOptions options)
    : MacDesktopCapturer(window, std::move(options))
    , _processor(std::make_unique<SCKProcessor>(screenQueueMaxLen(), videoFormat(window)))
{
    _processor->setOutputSink(this);
}

SCKCapturer::~SCKCapturer()
{
    _processor->stop();
    _processor->setOutputSink(nullptr);
}

bool SCKCapturer::available()
{
    if (@available(macOS 14.0, *)) {
        @autoreleasepool {
            return nil != NSClassFromString(@"SCStream");
        }
    }
    return false;
}

std::string SCKCapturer::selectedSource() const
{
    @autoreleasepool {
        if (auto screen = _processor->selectedScreen()) {
            return SCKEnumerator::displayToString(screen);
        }
        if (auto window = _processor->selectedWindow()) {
            return SCKEnumerator::windowToString(window);
        }
    }
    return {};
}

bool SCKCapturer::selectSource(const std::string& source)
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
                SCDisplay* display = _enumerator.toScreen(source);
                if (display) {
                    return _processor->selectDisplay(display);
                }
            }
        }
    }
    return false;
}

bool SCKCapturer::start()
{
    return _processor->start();
}

bool SCKCapturer::started() const
{
    return _processor->started();
}

void SCKCapturer::stop()
{
    _processor->stop();
}

void SCKCapturer::setPreviewMode(bool preview)
{
    _processor->setShowCursor(!preview);
}

void SCKCapturer::setTargetFramerate(int32_t fps)
{
    _processor->setTargetFramerate(fps);
}

void SCKCapturer::setTargetResolution(int32_t width, int32_t height)
{
    _processor->setTargetResolution(width, height);
}

void SCKCapturer::setExcludedWindow(webrtc::WindowId wId)
{
    if (!this->window() && enumerate()) {
        @autoreleasepool {
            SCWindow* window = _enumerator.toWindow(wId);
            if (window) {
                _processor->setExcludedWindow(window);
            }
        }
    }
}

bool SCKCapturer::enumerate()
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

void SCKCapturer::onCapturingError(std::string details, bool fatal)
{
    notifyAboutError(std::move(details), fatal);
}

void SCKCapturer::OnFrame(const webrtc::VideoFrame& frame)
{
    deliverCaptured(frame);
}

void SCKCapturer::OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c)
{
    processConstraints(c);
}

} // namespace LiveKitCpp
