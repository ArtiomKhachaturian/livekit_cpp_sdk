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
#include "DesktopWebRTCCapturerImpl.h"
#include <modules/desktop_capture/desktop_and_cursor_composer.h>
#ifdef RTC_ENABLE_WIN_WGC
#include <modules/desktop_capture/win/wgc_capturer_win.h>
#endif
#include <algorithm>
#include <cassert>

namespace LiveKitCpp
{

DesktopWebRTCCapturerImpl::DesktopWebRTCCapturerImpl(bool window)
    : _window(window)
    , _source(defaultSource(window))
{
}

DesktopWebRTCCapturerImpl::~DesktopWebRTCCapturerImpl()
{
    assert(!_capturer);
}

void DesktopWebRTCCapturerImpl::setCallback(webrtc::DesktopCapturer::Callback* callback)
{
    if (this != callback) {
        _callback = callback;
    }
}

void DesktopWebRTCCapturerImpl::start(webrtc::DesktopCaptureOptions options)
{
    std::unique_ptr<webrtc::DesktopCapturer> capturer;
#ifdef WEBRTC_WIN
    if (_window) {
        if (options.allow_wgc_window_capturer() && isWgcSupported(true)) {
            capturer = webrtc::WgcCapturerWin::CreateRawWindowCapturer(options);
        }
    }
    else {
        if (options.allow_wgc_screen_capturer() && isWgcSupported(false)) {
            capturer = webrtc::WgcCapturerWin::CreateRawScreenCapturer(options);
        }
    }
    if (capturer) {
        options.set_prefer_cursor_embedded(false);
    }
#endif
    if (!capturer) {
#ifdef WEBRTC_MAC
        options.set_allow_sck_capturer(false);
#endif
        if (_window) {
            capturer = webrtc::DesktopCapturer::CreateWindowCapturer(options);
        }
        else {
            capturer = webrtc::DesktopCapturer::CreateScreenCapturer(options);
        }
    }
    _capturer = std::move(capturer);
    if (_capturer) {
        if (options.prefer_cursor_embedded()) {
            _capturer = std::make_unique<webrtc::DesktopAndCursorComposer>(std::move(_capturer), options);
        }
        if (selectCapturerSource()) {
            _capturer->SetExcludedWindow(_excludeWindowId);
            _capturer->Start(this);
            if (_focusOnSelectedSource) {
                _capturer->FocusOnSelectedSource();
            }
        }
    }
    else {
        OnCaptureResult(webrtc::DesktopCapturer::Result::ERROR_PERMANENT, {});
    }
}

void DesktopWebRTCCapturerImpl::stop()
{
    _capturer.reset();
}

void DesktopWebRTCCapturerImpl::focusOnSelectedSource()
{
    if (!_focusOnSelectedSource) {
        _focusOnSelectedSource = true;
        if (_capturer) {
            _capturer->FocusOnSelectedSource();
        }
    }
}

void DesktopWebRTCCapturerImpl::setExcludedWindow(webrtc::WindowId window)
{
    if (_excludeWindowId != window) {
        _excludeWindowId = window;
        if (_capturer) {
            _capturer->SetExcludedWindow(window);
        }
    }
}

void DesktopWebRTCCapturerImpl::selectSource(intptr_t source)
{
    if (_source != source) {
        _source = source;
        selectCapturerSource();
    }
}

void DesktopWebRTCCapturerImpl::captureFrame()
{
    if (_capturer) {
        _capturer->CaptureFrame();
    }
}

intptr_t DesktopWebRTCCapturerImpl::defaultSource(bool window)
{
    if (window) {
        return webrtc::kNullWindowId;
    }
    return webrtc::kInvalidScreenId;
}

bool DesktopWebRTCCapturerImpl::selectCapturerSource()
{
    if (_capturer) {
        if (!_capturer->SelectSource(_source)) {
            OnCaptureResult(webrtc::DesktopCapturer::Result::ERROR_PERMANENT, {});
            return false;
        }
    }
    return true;
}

void DesktopWebRTCCapturerImpl::OnFrameCaptureStart()
{
    _callback.invoke(&webrtc::DesktopCapturer::Callback::OnFrameCaptureStart);
}

void DesktopWebRTCCapturerImpl::OnCaptureResult(webrtc::DesktopCapturer::Result result,
                                                std::unique_ptr<webrtc::DesktopFrame> frame)
{
    _callback.invoke(&webrtc::DesktopCapturer::Callback::OnCaptureResult,
                     result, std::move(frame));
}

} // namespace LiveKitCpp
