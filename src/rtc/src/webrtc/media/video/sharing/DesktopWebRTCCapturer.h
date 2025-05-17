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
#pragma once // CGDesktopCapturer.h
#include "DesktopSimpleCapturer.h"
#include "RtcObject.h"
#include <modules/desktop_capture/desktop_capturer.h>
#include "SafeObj.h"

namespace LiveKitCpp
{

class DesktopWebRTCCapturerImpl;

class DesktopWebRTCCapturer : public DesktopSimpleCapturer<DesktopCapturer>,
                              private RtcObject<DesktopWebRTCCapturerImpl, webrtc::DesktopCapturer::Callback>
{
    using ImplRef = std::weak_ptr<DesktopWebRTCCapturerImpl>;
public:
    DesktopWebRTCCapturer(bool window, bool previewMode,
                          webrtc::DesktopCaptureOptions options,
                          std::shared_ptr<webrtc::TaskQueueBase> timerQueue,
                          VideoFrameBufferPool framesPool = {});
    DesktopWebRTCCapturer(bool window, bool previewMode,
                          webrtc::DesktopCaptureOptions options,
                          VideoFrameBufferPool framesPool = {});
    ~DesktopWebRTCCapturer() override;
    // overrides & impl. of DesktopCapturer
    bool selectSource(const std::string& source) final;
    std::string selectedSource() const final { return _selectedSource(); }
    bool start() final;
    void stop() final;
    void focusOnSelectedSource() final;
    void setExcludedWindow(webrtc::WindowId window) final;
protected:
    void captureNextFrame() final;
    bool canStart() const final;
private:
    std::optional<intptr_t> parse(const std::string& source) const;
    bool hasValidSource() const;
    template <class Method, typename... Args>
    void postToImpl(Method method, Args&&... args) const;
    // impl. of webrtc::DesktopCapturer::Callback
    void OnFrameCaptureStart() final;
    void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    Bricks::SafeObj<std::string> _selectedSource;
};
	
} // namespace LiveKitCpp
