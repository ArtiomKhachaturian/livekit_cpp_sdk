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
#include "ScreenCaptureProcessor.h"
#include "ScreenCaptureProcessorImpl.h"

namespace LiveKitCpp
{

ScreenCaptureProcessor::ScreenCaptureProcessor(int queueDepth, OSType pixelFormat)
    : RtcObject<ScreenCaptureProcessorImpl>(queueDepth, pixelFormat)
{
}

ScreenCaptureProcessor::~ScreenCaptureProcessor()
{
    if (auto impl = dispose()) {
        impl->stop();
        impl->setOutputSink(nullptr);
    }
}

void ScreenCaptureProcessor::setOutputSink(CapturerProxySink* sink)
{
    if (const auto impl = loadImpl()) {
        impl->setOutputSink(sink);
    }
}

bool ScreenCaptureProcessor::start()
{
    const auto impl = loadImpl();
    return impl && impl->start();
}

bool ScreenCaptureProcessor::started() const
{
    const auto impl = loadImpl();
    return impl && impl->started();
}

void ScreenCaptureProcessor::stop()
{
    if (const auto impl = loadImpl()) {
        impl->stop();
    }
}

bool ScreenCaptureProcessor::selectDisplay(SCDisplay* display)
{
    if (display) {
        const auto impl = loadImpl();
        return impl && impl->selectDisplay(display);
    }
    return false;
}

bool ScreenCaptureProcessor::selectWindow(SCWindow* window)
{
    if (window) {
        const auto impl = loadImpl();
        return impl && impl->selectWindow(window);
    }
    return false;
}

void ScreenCaptureProcessor::setExcludedWindow(SCWindow* window)
{
    if (const auto impl = loadImpl()) {
        impl->setExcludedWindow(window);
    }
}

void ScreenCaptureProcessor::setShowCursor(bool show)
{
    if (const auto impl = loadImpl()) {
        impl->setShowCursor(show);
    }
}

void ScreenCaptureProcessor::setTargetFramerate(int32_t fps)
{
    if (const auto impl = loadImpl()) {
        impl->setTargetFramerate(fps);
    }
}

void ScreenCaptureProcessor::setTargetResolution(int32_t width, int32_t height)
{
    if (const auto impl = loadImpl()) {
        impl->setTargetResolution(width, height);
    }
}

SCDisplay* ScreenCaptureProcessor::selectedScreen() const
{
    if (const auto impl = loadImpl()) {
        return impl->selectedScreen();
    }
    return nil;
}

SCWindow* ScreenCaptureProcessor::selectedWindow() const
{
    if (const auto impl = loadImpl()) {
        return impl->selectedWindow();
    }
    return nil;
}

} // namespace LiveKitCpp
