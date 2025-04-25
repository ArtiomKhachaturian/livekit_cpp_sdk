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
#include "SCKProcessor.h"
#include "SCKProcessorImpl.h"

namespace LiveKitCpp
{

SCKProcessor::SCKProcessor(bool previewMode, VideoFrameBufferPool framesPool)
    : RtcObject<SCKProcessorImpl>(previewMode, std::move(framesPool))
{
}

SCKProcessor::~SCKProcessor()
{
    if (auto impl = dispose()) {
        impl->stop();
        impl->setOutputSink(nullptr);
    }
}

void SCKProcessor::setOutputSink(CapturerProxySink* sink)
{
    if (const auto impl = loadImpl()) {
        impl->setOutputSink(sink);
    }
}

bool SCKProcessor::start()
{
    const auto impl = loadImpl();
    return impl && impl->start();
}

bool SCKProcessor::started() const
{
    const auto impl = loadImpl();
    return impl && impl->started();
}

void SCKProcessor::stop()
{
    if (const auto impl = loadImpl()) {
        impl->stop();
    }
}

bool SCKProcessor::selectDisplay(SCDisplay* display)
{
    if (display) {
        const auto impl = loadImpl();
        return impl && impl->selectDisplay(display);
    }
    return false;
}

bool SCKProcessor::selectWindow(SCWindow* window)
{
    if (window) {
        const auto impl = loadImpl();
        return impl && impl->selectWindow(window);
    }
    return false;
}

void SCKProcessor::setExcludedWindow(SCWindow* window)
{
    if (const auto impl = loadImpl()) {
        impl->setExcludedWindow(window);
    }
}

void SCKProcessor::focusOnSelectedWindow()
{
    if (const auto impl = loadImpl()) {
        impl->focusOnSelectedWindow();
    }
}

void SCKProcessor::setShowCursor(bool show)
{
    if (const auto impl = loadImpl()) {
        impl->setShowCursor(show);
    }
}

void SCKProcessor::setTargetFramerate(int32_t fps)
{
    if (const auto impl = loadImpl()) {
        impl->setTargetFramerate(fps);
    }
}

void SCKProcessor::setTargetResolution(int32_t width, int32_t height)
{
    if (const auto impl = loadImpl()) {
        impl->setTargetResolution(width, height);
    }
}

SCDisplay* SCKProcessor::selectedDisplay() const
{
    if (const auto impl = loadImpl()) {
        return impl->selectedDisplay();
    }
    return nil;
}

SCWindow* SCKProcessor::selectedWindow() const
{
    if (const auto impl = loadImpl()) {
        return impl->selectedWindow();
    }
    return nil;
}

} // namespace LiveKitCpp
