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

CGDesktopCapturer::CGDesktopCapturer(const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue,
                                     bool window, const webrtc::DesktopCaptureOptions& options)
    : Base(timerQueue, window, options.configuration_monitor())
    , _options(options)
{
    if (window) {
        _source = webrtc::kNullWindowId;
    }
    else {
        _source = webrtc::kInvalidScreenId;
    }
    setTargetFramerate(DesktopConfiguration::maxFramerate(window));
}

CGDesktopCapturer::~CGDesktopCapturer()
{
    stop();
}

void CGDesktopCapturer::setPreviewMode(bool preview)
{
    LOCK_WRITE_SAFE_OBJ(_cursorComposer);
    if (preview) {
        _cursorComposer->reset();
    }
    else if (!_cursorComposer->get()) {
        _cursorComposer->reset(new DesktopCursorComposer(this, _options));
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
        return validSource(window(), _source);
    }
    return false;
}

void CGDesktopCapturer::captureNextFrame()
{
    std::unique_ptr<webrtc::DesktopFrame> frame;
    if (window()) {
        frame = captureWindow(static_cast<webrtc::WindowId>(_source));
    }
    else {
        frame = captureDisplay(static_cast<webrtc::ScreenId>(_source));
    }
    if (frame) {
        LOCK_READ_SAFE_OBJ(_cursorComposer);
        if (const auto& composer = _cursorComposer.constRef()) {
            composer->setFrame(std::move(frame));
        }
        else {
            deliverCaptured(std::move(frame));
        }
    }
}

bool CGDesktopCapturer::canStart() const
{
    return Base::canStart() && validSource(window(), _source);
}

bool CGDesktopCapturer::validSource(bool window, intptr_t source)
{
    if (window) {
        return webrtc::kNullWindowId != source;
    }
    return webrtc::kInvalidScreenId != source && webrtc::kInvalidDisplayId != source;
}

void CGDesktopCapturer::OnCaptureResult(webrtc::DesktopAndCursorComposer::Result result,
                                        std::unique_ptr<webrtc::DesktopFrame> frame)
{
    if (frame) {
        deliverCaptured(std::move(frame));
    }
    else if (webrtc::DesktopAndCursorComposer::Result::ERROR_PERMANENT == result) {
        notifyAboutFatalError();
    }
}

} // namespace LiveKitCpp
