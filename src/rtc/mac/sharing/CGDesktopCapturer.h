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
#include "DesktopCursorComposer.h"
#include "DesktopSimpleCapturer.h"
#include "MacDesktopCapturer.h"
#include "SafeObj.h"
#include <atomic>

namespace webrtc {
class MouseCursorMonitor;
}

namespace LiveKitCpp
{

class CGDesktopCapturer : public DesktopSimpleCapturer<MacDesktopCapturer>,
                          private webrtc::DesktopCapturer::Callback
{
    using Base = DesktopSimpleCapturer<MacDesktopCapturer>;
public:
    CGDesktopCapturer(const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue,
                      bool window, const webrtc::DesktopCaptureOptions& options);
    ~CGDesktopCapturer() override;
    // overrides & impl. of DesktopCapturer
    void setPreviewMode(bool preview) final;
    bool selectSource(const std::string& source) final;
protected:
    void captureNextFrame() final;
    bool canStart() const final;
private:
    static bool validSource(bool window, intptr_t source);
    std::unique_ptr<webrtc::DesktopFrame> processFrame(std::unique_ptr<webrtc::DesktopFrame> frame) const;
    // impl. of webrtc::DesktopCapturer::Callback
    void OnCaptureResult(webrtc::DesktopAndCursorComposer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    const webrtc::DesktopCaptureOptions _options;
    std::atomic<intptr_t> _source;
    Bricks::SafeUniquePtr<DesktopCursorComposer> _cursorComposer;
};
	
} // namespace LiveKitCpp
