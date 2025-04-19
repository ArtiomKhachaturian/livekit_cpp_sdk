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
#include "CGDesktopCapturer.h"
#include "DesktopConfiguration.h"
#include "Utils.h"
#include <modules/desktop_capture/desktop_capture_options.h>

namespace LiveKitCpp
{

CGDesktopCapturer::CGDesktopCapturer(bool window, const webrtc::DesktopCaptureOptions& options,
                                     const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue)
    : MacDesktopCapturer(window, options.configuration_monitor())
    , _timer(timerQueue, this)
{
    if (window) {
        _source = webrtc::kNullWindowId;
    }
    else {
        _source = webrtc::kInvalidScreenId;
    }
    _frameRate = DesktopConfiguration::maxFramerate(window);
}

CGDesktopCapturer::~CGDesktopCapturer()
{
    _timer.stop();
    _timer.setCallback(nullptr);
}

void CGDesktopCapturer::setPreviewMode(bool preview)
{
    if (exchangeVal(preview, _preview)) {
        
    }
}

void CGDesktopCapturer::setTargetFramerate(int32_t fps)
{
    if (exchangeVal(fps, _frameRate) && _timer.started()) {
        _timer.stop();
        _timer.start(float(fps));
    }
}

bool CGDesktopCapturer::selectSource(const std::string& source)
{
    if (!started()) {
        if (window()) {
            const auto id = windowIdFromString(source);
            if (id.has_value()) {
                _source = id.value();
            }
        }
        const auto id = screenIdFromString(source);
        if (id.has_value()) {
            _source = id.value();
        }
        return hasValidSource();
    }
    return false;
}

bool CGDesktopCapturer::start()
{
    if (!started() && hasValidSource() && _frameRate > 0) {
        _timer.start(float(_frameRate));
        return true;
    }
    return false;
}

bool CGDesktopCapturer::validSource(bool window, intptr_t source)
{
    if (window) {
        return webrtc::kNullWindowId != source;
    }
    return webrtc::kInvalidScreenId != source && webrtc::kInvalidDisplayId != source;
}

void CGDesktopCapturer::onTimeout(uint64_t)
{
    std::unique_ptr<webrtc::DesktopFrame> frame;
    if (window()) {
        frame = captureWindow(static_cast<webrtc::WindowId>(_source));
    }
    else {
        frame = captureDisplay(static_cast<webrtc::ScreenId>(_source));
    }
    if (frame && !_preview) {
        // TODO: apply cursor
    }
    deliverCaptured(std::move(frame));
}

} // namespace LiveKitCpp
