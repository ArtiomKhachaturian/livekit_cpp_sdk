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
#include "DesktopWebRTCCursorComposer.h"
#include "DesktopSimpleCapturer.h"
#include "MacDesktopCapturer.h"
#include "SafeObj.h"
#include <atomic>

namespace webrtc {
class MouseCursorMonitor;
}

namespace LiveKitCpp
{

class CGScreenCapturer : public DesktopSimpleCapturer<MacDesktopCapturer>,
                         private webrtc::DesktopCapturer::Callback
{
    using Base = DesktopSimpleCapturer<MacDesktopCapturer>;
public:
    CGScreenCapturer(bool previewMode, webrtc::DesktopCaptureOptions options, VideoFrameBufferPool framesPool = {});
    CGScreenCapturer(bool previewMode, webrtc::DesktopCaptureOptions options,
                     std::shared_ptr<webrtc::TaskQueueBase> timerQueue,
                     VideoFrameBufferPool framesPool = {});
    ~CGScreenCapturer() final;
    // overrides & impl. of DesktopCapturer
    bool selectSource(const std::string& source) final;
    std::string selectedSource() const final { return screenIdToString(_source); }
protected:
    void captureNextFrame() final;
    bool canStart() const final;
private:
    webrtc::DesktopVector dpi() const;
    std::unique_ptr<webrtc::DesktopFrame> processFrame(std::unique_ptr<webrtc::DesktopFrame> frame) const;
    // impl. of webrtc::DesktopCapturer::Callback
    void OnCaptureResult(webrtc::DesktopAndCursorComposer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
private:
    const std::unique_ptr<DesktopWebRTCCursorComposer> _cursorComposer;
    std::atomic<webrtc::ScreenId> _source = webrtc::kInvalidScreenId;
};
	
} // namespace LiveKitCpp
