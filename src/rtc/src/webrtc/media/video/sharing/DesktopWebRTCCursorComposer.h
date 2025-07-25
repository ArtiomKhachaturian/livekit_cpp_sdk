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
#pragma once // DesktopCursorComposer.h
#include "Listener.h"
#include <modules/desktop_capture/desktop_and_cursor_composer.h>

namespace LiveKitCpp
{

class DesktopWebRTCCursorComposer : private webrtc::DesktopCapturer::Callback
{
    class Proxy;
public:
    DesktopWebRTCCursorComposer(const webrtc::DesktopCaptureOptions& options,
                                webrtc::DesktopCapturer::Callback* callback = nullptr);
    void setFrame(std::unique_ptr<webrtc::DesktopFrame> frame);
    void setCallback(webrtc::DesktopCapturer::Callback* callback);
private:
    // impl. of webrtc::DesktopCapturer::Callback
    void OnFrameCaptureStart() final;
    void OnCaptureResult(webrtc::DesktopAndCursorComposer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    Proxy* const _proxy;
    Bricks::Listener<webrtc::DesktopCapturer::Callback*> _callback;
    webrtc::DesktopAndCursorComposer _composer;
};
	
} // namespace LiveKitCpp
