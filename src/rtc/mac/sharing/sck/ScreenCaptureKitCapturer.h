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
#pragma once // ScreenCaptureKitCapturer.h
#include "MacDesktopCapturer.h"
#include "ScreenCaptureEnumerator.h"
#include "CapturerProxySink.h"

namespace LiveKitCpp
{

class ScreenCaptureProcessor;

class ScreenCaptureKitCapturer : public MacDesktopCapturer,
                                 private CapturerProxySink
{
public:
    ScreenCaptureKitCapturer(bool window, webrtc::DesktopCaptureOptions options);
    ~ScreenCaptureKitCapturer() override;
    static bool available();
    // overrides of DesktopCapturer
    bool selectSource(const std::string& source) final;
    std::string selectedSource() const final;
    bool start() final;
    bool started() const final;
    void stop() final;
    void setPreviewMode(bool preview) final;
    void setTargetFramerate(int32_t fps) final;
    void setTargetResolution(int32_t width, int32_t height) final;
    void setExcludedWindow(webrtc::WindowId window) final;
private:
    bool enumerate();
    // impl. of CapturerProxySink
    void onStateChanged(CapturerState state) final { changeState(state); }
    void onCapturingError(std::string details, bool fatal) final;
    void OnFrame(const webrtc::VideoFrame& frame) final;
    void OnDiscardedFrame() final { discardFrame(); }
    void OnConstraintsChanged(const webrtc::VideoTrackSourceConstraints& c) final;
private:
    const std::unique_ptr<ScreenCaptureProcessor> _processor;
    ScreenCaptureEnumerator _enumerator;
};

} // namespace LiveKitCpp
