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
#pragma once // AsyncSharingSourceImpl.h
#include "AsyncVideoSourceImpl.h"
#include "MediaTimer.h"
#include <modules/desktop_capture/desktop_capturer.h>

namespace LiveKitCpp
{

class AsyncSharingSourceImpl : public AsyncVideoSourceImpl,
                               private webrtc::DesktopCapturer::Callback,
                               private MediaTimerCallback
{
public:
    AsyncSharingSourceImpl(bool windowCapturer,
                           std::unique_ptr<webrtc::DesktopCapturer> capturer,
                           std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                           const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue,
                           const std::shared_ptr<Bricks::Logger>& logger);
    ~AsyncSharingSourceImpl() final;
    // override of AsyncVideoSourceImpl
    void requestCapturer() final;
    void resetCapturer() final;
private:
    // impl. of webrtc::DesktopCapturer::Callback
    void OnFrameCaptureStart() final;
    void OnCaptureResult(webrtc::DesktopCapturer::Result result,
                         std::unique_ptr<webrtc::DesktopFrame> frame) final;
    // impl. of MediaTimerCallback
    void onTimeout(uint64_t) final;
private:
    const bool _windowCapturer;
    const std::unique_ptr<webrtc::DesktopCapturer> _capturer;
    MediaTimer _eventsTimer;
};
	
} // namespace LiveKitCpp
