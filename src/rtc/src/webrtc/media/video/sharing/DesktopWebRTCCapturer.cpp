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
#include "DesktopCapturerUtils.h"
#include "Utils.h"
#include <modules/desktop_capture/desktop_capture_options.h>
#include <modules/desktop_capture/desktop_and_cursor_composer.h>
#ifdef RTC_ENABLE_WIN_WGC
#include <modules/desktop_capture/win/wgc_capturer_win.h>
#endif

namespace LiveKitCpp
{

DesktopWebRTCCapturer::DesktopWebRTCCapturer(bool window, bool previewMode,
                                             webrtc::DesktopCaptureOptions options,
                                             std::shared_ptr<webrtc::TaskQueueBase> timerQueue,
                                             VideoFrameBufferPool framesPool)
    : Base(window, previewMode, std::move(options), std::move(timerQueue), std::move(framesPool))
    , _source(defaultSource(window))
{
}

DesktopWebRTCCapturer::DesktopWebRTCCapturer(bool window, bool previewMode,
                                             webrtc::DesktopCaptureOptions options,
                                             VideoFrameBufferPool framesPool)
    : Base(window, previewMode, std::move(options), std::move(framesPool))
    , _source(defaultSource(window))
{
}

DesktopWebRTCCapturer::~DesktopWebRTCCapturer()
{
    stop();
}

bool DesktopWebRTCCapturer::selectSource(const std::string& source)
{
    if (!started()) {
        auto sourceId = parse(source);
        if (sourceId && exchangeVal(sourceId.value(), _source)) {
            _focusOnSelectedSource = false;
        }
        return hasValidSource();
    }
    return false;
}

std::string DesktopWebRTCCapturer::selectedSource() const
{
    return window() ? windowIdToString(_source) : screenIdToString(_source);
}

void DesktopWebRTCCapturer::stop()
{
    if (auto capturer = webRtcCapturer(true)) { 
        execute([capturer = std::move(capturer)]() mutable { capturer.reset(); });
    }
    Base::stop();
}

void DesktopWebRTCCapturer::focusOnSelectedSource()
{
    if (!previewMode() && !_focusOnSelectedSource.exchange(true)) {
        LOCK_READ_SAFE_OBJ(_capturer);
        if (const auto& capturer = _capturer.constRef()) {
            execute([capturerRef = CapturerWeak(capturer)]() {
                if (const auto capturer = capturerRef.lock()) {
                    capturer->FocusOnSelectedSource();
                }
            });
        }
    }
}

void DesktopWebRTCCapturer::setExcludedWindow(webrtc::WindowId window)
{
    if (exchangeVal(window, _excludeWindowId)) {
        LOCK_READ_SAFE_OBJ(_capturer);
        if (const auto& capturer = _capturer.constRef()) {
            execute([window, capturerRef = CapturerWeak(capturer)]() {
                if (const auto capturer = capturerRef.lock()) {
                    capturer->SetExcludedWindow(window);
                }
            });
        }
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
    else {
        _smf(std::move(smf));
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
    return Base::canStart() && hasValidSource();
}

intptr_t DesktopWebRTCCapturer::defaultSource(bool window)
{
    if (window) {
        return webrtc::kNullWindowId;
    }
    return webrtc::kInvalidScreenId;
}

std::unique_ptr<webrtc::DesktopCapturer> DesktopWebRTCCapturer::
    createCapturer(bool window, webrtc::DesktopCaptureOptions& options)
{
    std::unique_ptr<webrtc::DesktopCapturer> capturer;
#ifdef WEBRTC_WIN
    if (window) {
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
        if (window) {
            capturer = webrtc::DesktopCapturer::CreateWindowCapturer(options);
        }
        else {
            capturer = webrtc::DesktopCapturer::CreateScreenCapturer(options);
        }
    }
    return capturer;
}

std::shared_ptr<webrtc::DesktopCapturer> DesktopWebRTCCapturer::webRtcCapturer(bool take)
{
    if (take) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        return _capturer.take();
    }
    if (hasValidSource()) {
        LOCK_WRITE_SAFE_OBJ(_capturer);
        if (!_capturer->get()) {
            auto capturerOptions = options();
            auto capturer = createCapturer(window(), capturerOptions);
            if (capturer) {
                if (capturerOptions.prefer_cursor_embedded()) {
                    capturer = std::make_unique<webrtc::DesktopAndCursorComposer>(std::move(capturer), options());
                }
                if (capturer->SelectSource(_source)) {
                    capturer->SetExcludedWindow(_excludeWindowId);
                    capturer->SetMaxFrameRate(targetFramerate());
                    {
                        LOCK_WRITE_SAFE_OBJ(_smf);
                        capturer->SetSharedMemoryFactory(_smf.take());
                    }
                    capturer->Start(this);
                    if (_focusOnSelectedSource) {
                        capturer->FocusOnSelectedSource();
                    }
                    _capturer->reset(capturer.release());
                }
                else {
                    notifyAboutError("failed to select capturer source");
                }
            }
            else {
                notifyAboutError("failed to create platform capturer");
            }
        }
        return _capturer.constRef();
    }
    return nullptr;
}

std::optional<intptr_t> DesktopWebRTCCapturer::parse(const std::string& source) const
{
    if (window()) {
        return windowIdFromString(source);
    }
    return screenIdFromString(source);
}

bool DesktopWebRTCCapturer::hasValidSource() const
{
    const auto source = _source.load();
    if (window()) {
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
    else {
        notifyAboutError("capture has failed", webrtc::DesktopCapturer::Result::ERROR_PERMANENT == result);
    }
}

} // namespace LiveKitCpp
