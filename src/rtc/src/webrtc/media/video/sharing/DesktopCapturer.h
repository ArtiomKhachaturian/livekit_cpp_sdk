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
#pragma once // DesktopCapturer.h
#include "CapturerState.h"
#include "Listener.h"
#include <api/video/video_frame.h>
#include <modules/desktop_capture/desktop_frame.h>
#include <atomic>

namespace LiveKitCpp
{

class CapturerProxySink;

class DesktopCapturer
{
public:
    bool window() const noexcept { return _window; }
    void setOutputSink(CapturerProxySink* sink);
    virtual ~DesktopCapturer() = default;
protected:
    DesktopCapturer(bool window);
    bool hasOutputSink() const { return !_sink.empty(); }
    bool changeState(CapturerState state);
    void notifyAboutFatalError(const std::string& details = {}) const;
    void deliverCaptured(const webrtc::VideoFrame& frame);
    void deliverCaptured(const rtc::scoped_refptr<webrtc::VideoFrameBuffer>& buff,
                         int64_t timeStampMicro = 0LL,
                         webrtc::VideoRotation rotation = webrtc::VideoRotation::kVideoRotation_0,
                         const std::optional<webrtc::ColorSpace>& colorSpace = {});
    void deliverCaptured(std::unique_ptr<webrtc::DesktopFrame> frame,
                         const std::optional<webrtc::ColorSpace>& colorSpace = {});
private:
    const bool _window;
    Bricks::Listener<CapturerProxySink*> _sink;
    std::atomic<uint16_t> _lastFrameId = 0U;
    Bricks::SafeObj<CapturerState> _state = CapturerState::Stopped;
};
	
} // namespace LiveKitCpp
