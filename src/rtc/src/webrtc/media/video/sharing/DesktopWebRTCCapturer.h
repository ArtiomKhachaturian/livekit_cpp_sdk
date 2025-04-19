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
#include <modules/desktop_capture/desktop_capturer.h>
#include <modules/desktop_capture/desktop_capture_options.h>
#include "SafeObj.h"
#include <atomic>

namespace LiveKitCpp
{

class DesktopWebRTCCapturer : public DesktopSimpleCapturer<DesktopCapturer>,
                              private webrtc::DesktopCapturer::Callback
{
    using Base = DesktopSimpleCapturer<DesktopCapturer>;
    using CapturerWeak = std::weak_ptr<webrtc::DesktopCapturer>;
public:
    DesktopWebRTCCapturer(const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue,
                          bool window, const webrtc::DesktopCaptureOptions& options);
    ~DesktopWebRTCCapturer() override;
    // overrides & impl. of DesktopCapturer
    void setPreviewMode(bool preview) final { _previewMode = preview; }
    bool selectSource(const std::string& source) final;
    std::string selectedSource() const final;
    void stop() final;
    bool focusOnSelectedSource() final;
    void setExcludedWindow(webrtc::WindowId window) final;
    void setSharedMemoryFactory(std::unique_ptr<webrtc::SharedMemoryFactory> smf) final;
protected:
    void captureNextFrame() final;
    bool canStart() const final;
private:
    std::shared_ptr<webrtc::DesktopCapturer> webRtcCapturer(bool take = false);
    bool validSource() const { return validSource(window(), _source); }
    static bool validSource(bool window, intptr_t source);
    // impl. of webrtc::DesktopCapturer::Callback
    void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    Bricks::SafeSharedPtr<webrtc::DesktopCapturer> _capturer;
    std::atomic<intptr_t> _source;
    std::atomic_bool _previewMode = false;
};
	
} // namespace LiveKitCpp
