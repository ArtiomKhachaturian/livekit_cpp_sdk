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
#pragma once // DesktopWebRTCCapturerImpl.h
#include "Listener.h"
#include <modules/desktop_capture/desktop_capturer.h>
#include <modules/desktop_capture/desktop_capture_options.h>

namespace LiveKitCpp
{

class DesktopWebRTCCapturerImpl : private webrtc::DesktopCapturer::Callback
{
public:
    DesktopWebRTCCapturerImpl(bool window);
    ~DesktopWebRTCCapturerImpl() final;
    void setCallback(webrtc::DesktopCapturer::Callback* callback);
    void start(webrtc::DesktopCaptureOptions options);
    void stop();
    void focusOnSelectedSource();
    void setExcludedWindow(webrtc::WindowId window);
    void selectSource(intptr_t source);
    void captureFrame();
private:
    static intptr_t defaultSource(bool window);
    bool selectCapturerSource();
    // impl. of webrtc::DesktopCapturer::Callback
    void OnFrameCaptureStart() final;
    void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    const bool _window;
    Bricks::Listener<webrtc::DesktopCapturer::Callback*> _callback;
    std::unique_ptr<webrtc::DesktopCapturer> _capturer;
    intptr_t _source;
    bool _focusOnSelectedSource = false;
    webrtc::WindowId _excludeWindowId = webrtc::kNullWindowId;
};

} // namespace LiveKitCpp
