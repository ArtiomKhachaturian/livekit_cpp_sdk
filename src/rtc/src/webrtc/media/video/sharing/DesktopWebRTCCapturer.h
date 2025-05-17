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
    std::string selectedSource() const final;
    void stop() final;
    void focusOnSelectedSource() final;
    void setExcludedWindow(webrtc::WindowId window) final;
    void setSharedMemoryFactory(std::unique_ptr<webrtc::SharedMemoryFactory> smf) final;
protected:
    void captureNextFrame() final;
    bool canStart() const final;
private:
    static intptr_t defaultSource(bool window);
    static std::unique_ptr<webrtc::DesktopCapturer>
        createCapturer(bool window, webrtc::DesktopCaptureOptions& options);
    static std::unique_ptr<webrtc::DesktopCapturer>
        startCapturer(std::unique_ptr<webrtc::DesktopCapturer> capturer,
                      std::unique_ptr<webrtc::SharedMemoryFactory> smf,
                      const webrtc::DesktopCaptureOptions& options,
                      webrtc::DesktopCapturer::Callback* callback,
                      int32_t maxFramerate,
                      intptr_t source,
                      webrtc::WindowId excludeWindowId,
                      bool focusOnSelectedSource);
    std::unique_ptr<webrtc::SharedMemoryFactory> takeSmf();
    std::shared_ptr<webrtc::DesktopCapturer> takeCapturer();
    std::shared_ptr<webrtc::DesktopCapturer> ensureCapturer();
    std::weak_ptr<webrtc::DesktopCapturer> capturer() const;
    std::optional<intptr_t> parse(const std::string& source) const;
    bool hasValidSource() const;
    // impl. of webrtc::DesktopCapturer::Callback
    void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    Bricks::SafeSharedPtr<webrtc::DesktopCapturer> _capturer;
    Bricks::SafeUniquePtr<webrtc::SharedMemoryFactory> _smf;
    std::atomic<intptr_t> _source;
    std::atomic_bool _focusOnSelectedSource = false;
    std::atomic<webrtc::WindowId> _excludeWindowId = webrtc::kNullWindowId;
};
	
} // namespace LiveKitCpp
