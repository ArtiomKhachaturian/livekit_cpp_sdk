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
#include "DesktopWebRTCCursorComposer.h"

namespace LiveKitCpp
{

class DesktopWebRTCCursorComposer::Proxy : public webrtc::DesktopCapturer
{
public:
    Proxy() = default;
    void setFrame(std::unique_ptr<webrtc::DesktopFrame> frame);
    // impl. of webrtc::DesktopCapturer
    void Start(webrtc::DesktopCapturer::Callback* callback) final { _callback = callback; }
    void CaptureFrame() final;
private:
    webrtc::DesktopCapturer::Callback* _callback = nullptr;
    std::unique_ptr<webrtc::DesktopFrame> _frame;
};

DesktopWebRTCCursorComposer::DesktopWebRTCCursorComposer(const webrtc::DesktopCaptureOptions& options,
                                                         webrtc::DesktopCapturer::Callback* callback)
    : _callback(callback)
    , _proxy(new Proxy)
    , _composer(std::unique_ptr<webrtc::DesktopCapturer>(_proxy), options)
{
    _composer.Start(this);
}

void DesktopWebRTCCursorComposer::setFrame(std::unique_ptr<webrtc::DesktopFrame> frame)
{
    if (frame) {
        _proxy->setFrame(std::move(frame));
        _composer.CaptureFrame();
    }
}

void DesktopWebRTCCursorComposer::setCallback(webrtc::DesktopCapturer::Callback* callback)
{
    _callback = callback;
}

void DesktopWebRTCCursorComposer::OnFrameCaptureStart()
{
    _callback.invoke(&webrtc::DesktopCapturer::Callback::OnFrameCaptureStart);
}

void DesktopWebRTCCursorComposer::OnCaptureResult(webrtc::DesktopAndCursorComposer::Result result,
                                                  std::unique_ptr<webrtc::DesktopFrame> frame)
{
    _callback.invoke(&webrtc::DesktopCapturer::Callback::OnCaptureResult, result, std::move(frame));
}

void DesktopWebRTCCursorComposer::Proxy::setFrame(std::unique_ptr<webrtc::DesktopFrame> frame)
{
    _frame = std::move(frame);
}

void DesktopWebRTCCursorComposer::Proxy::CaptureFrame()
{
    if (_frame && _callback) {
        _callback->OnCaptureResult(webrtc::DesktopAndCursorComposer::Result::SUCCESS,
                                   std::move(_frame));
    }
}

} // namespace LiveKitCpp
