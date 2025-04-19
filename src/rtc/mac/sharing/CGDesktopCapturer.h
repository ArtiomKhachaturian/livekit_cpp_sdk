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
#include "MacDesktopCapturer.h"
#include "MediaTimer.h"
#include <atomic>

namespace LiveKitCpp
{

class CGDesktopCapturer : public MacDesktopCapturer,
                          private MediaTimerCallback
{
public:
    CGDesktopCapturer(bool window, const webrtc::DesktopCaptureOptions& options,
                      const std::shared_ptr<webrtc::TaskQueueBase>& timerQueue);
    ~CGDesktopCapturer() final;
    // overrides & impl. of DesktopCapturer
    void setPreviewMode(bool preview) final;
    void setTargetFramerate(int32_t fps) final;
    bool selectSource(const std::string& source) final;
    bool started() const final { return _timer.started(); }
    bool start() final;
    void stop() { _timer.stop(); }
private:
    static bool validSource(bool window, intptr_t source);
    bool hasValidSource() const { return validSource(window(), _source); }
    // impl. of MediaTimerCallback
    void onTimeout(uint64_t) final;
private:
    MediaTimer _timer;
    std::atomic_bool _preview = false;
    std::atomic<intptr_t> _source;
    std::atomic<int32_t> _frameRate;
};
	
} // namespace LiveKitCpp
