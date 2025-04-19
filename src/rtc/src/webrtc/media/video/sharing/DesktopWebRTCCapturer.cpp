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
#include "DesktopWebRTCCapturer.h"
#include "DesktopConfiguration.h"
#include "DesktopCapturerUtils.h"
#include "Utils.h"
#include <modules/desktop_capture/desktop_capture_options.h>
#include <modules/desktop_capture/desktop_and_cursor_composer.h>

namespace LiveKitCpp
{

DesktopWebRTCCapturer::DesktopWebRTCCapturer(const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue,
                                             bool window, const webrtc::DesktopCaptureOptions& options)
    : Base(timerQueue, window, options)
{
    if (window) {
        _source = webrtc::kNullWindowId;
    }
    else {
        _source = webrtc::kInvalidScreenId;
    }
    setTargetFramerate(DesktopConfiguration::maxFramerate(window));
}

DesktopWebRTCCapturer::~DesktopWebRTCCapturer()
{
    stop();
}

bool DesktopWebRTCCapturer::selectSource(const std::string& source)
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
        return validSource();
    }
    return false;
}

std::string DesktopWebRTCCapturer::selectedSource() const
{
    return window() ? windowIdToString(_source) : screenIdToString(_source);
}

void DesktopWebRTCCapturer::stop()
{
    execute([capturer = webRtcCapturer(true)]() mutable { capturer.reset(); });
    Base::stop();
}

bool DesktopWebRTCCapturer::focusOnSelectedSource()
{
    LOCK_READ_SAFE_OBJ(_capturer);
    if (const auto& capturer = _capturer.constRef()) {
        execute([capturerRef = CapturerWeak(capturer)]() {
            if (const auto capturer = capturerRef.lock()) {
                capturer->FocusOnSelectedSource();
            }
        });
        return true;
    }
    return false;
}

void DesktopWebRTCCapturer::setExcludedWindow(webrtc::WindowId window)
{
    LOCK_READ_SAFE_OBJ(_capturer);
    if (const auto& capturer = _capturer.constRef()) {
        execute([window, capturerRef = CapturerWeak(capturer)]() {
            if (const auto capturer = capturerRef.lock()) {
                capturer->SetExcludedWindow(window);
            }
        });
    }
}

void DesktopWebRTCCapturer::setSharedMemoryFactory(std::unique_ptr<webrtc::SharedMemoryFactory> smf)
{
    LOCK_READ_SAFE_OBJ(_capturer);
    if (const auto& capturer = _capturer.constRef()) {
        execute([smf = std::move(smf), capturerRef = CapturerWeak(capturer)]() mutable {
            if (const auto capturer = capturerRef.lock()) {
                capturer->SetSharedMemoryFactory(std::move(smf));
            }
        });
    }
}

void DesktopWebRTCCapturer::captureNextFrame()
{
    if (const auto capturer = webRtcCapturer(false)) {
        capturer->CaptureFrame();
    }
}

bool DesktopWebRTCCapturer::canStart() const
{
    return Base::canStart() && validSource(window(), _source);
}

std::shared_ptr<webrtc::DesktopCapturer> DesktopWebRTCCapturer::webRtcCapturer(bool take)
{
    if (take) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        return _capturer.take();
    }
    if (validSource()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer->get()) {
            std::unique_ptr<webrtc::DesktopCapturer> capturer;
            if (window()) {
                capturer = webrtc::DesktopCapturer::CreateWindowCapturer(options());
            }
            else {
                capturer = webrtc::DesktopCapturer::CreateScreenCapturer(options());
            }
            if (capturer) {
                if (!_previewMode) {
                    capturer = std::make_unique<webrtc::DesktopAndCursorComposer>(std::move(capturer), options());
                }
                if (capturer->SelectSource(_source)) {
                    capturer->SetMaxFrameRate(targetFramerate());
                    capturer->Start(this);
                    _capturer->reset(capturer.release());
                }
                else {
                    notifyAboutFatalError("failed to select capturer source");
                }
            }
            else {
                notifyAboutFatalError("failed to create platform capturer");
            }
        }
        return _capturer.constRef();
    }
    return nullptr;
}

bool DesktopWebRTCCapturer::validSource(bool window, intptr_t source)
{
    if (window) {
        return webrtc::kNullWindowId != source;
    }
    return webrtc::kInvalidScreenId != source && webrtc::kInvalidDisplayId != source;
}

void DesktopWebRTCCapturer::OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                            std::unique_ptr<webrtc::DesktopFrame> frame)
{
    if (frame) {
        deliverCaptured(std::move(frame));
    }
    else if (webrtc::DesktopCapturer::Result::ERROR_PERMANENT == result) {
        notifyAboutFatalError("capture has failed and will keep failing again");
    }
}

} // namespace LiveKitCpp
