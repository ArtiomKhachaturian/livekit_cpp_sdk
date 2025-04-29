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
#pragma once // DesktopTimedCapturer.h
#include "DesktopCapturer.h"
#include "DesktopConfiguration.h"
#include "MediaTimer.h"
#include "SafeObj.h"
#include "VideoUtils.h"
#include "RtcUtils.h"
#include "Utils.h"
#include <atomic>
#include <type_traits>

namespace LiveKitCpp
{

// timer-based capturer
template <class TCapturer>
class DesktopSimpleCapturer : public TCapturer,
                              private MediaTimerCallback
{
    static_assert(std::is_base_of_v<DesktopCapturer, TCapturer>);
public:
    // overrides & impl. of DesktopCapturer
    int32_t targetFramerate() const noexcept { return _fps; }
    void setTargetFramerate(int32_t fps) final;
    bool started() const final { return _timer.started(); }
    bool start() override;
    void stop() override;
    ~DesktopSimpleCapturer() override;
protected:
    // capturer with shared queue
    template <typename... Args>
    DesktopSimpleCapturer(bool window, bool previewMode,
                          webrtc::DesktopCaptureOptions options,
                          std::shared_ptr<webrtc::TaskQueueBase> timerQueue,
                          VideoFrameBufferPool framesPool,
                          Args&&... args);
    // capturer with own queue
    template <typename... Args>
    DesktopSimpleCapturer(bool window, bool previewMode,
                          webrtc::DesktopCaptureOptions options,
                          VideoFrameBufferPool framesPool,
                          Args&&... args);
    void execute(absl::AnyInvocable<void()&&> task, uint64_t delayMs = 0ULL);
    virtual void captureNextFrame() = 0;
    virtual bool canStart() const { return _fps > 0; }
private:
    static std::string formatQueueName(const DesktopSimpleCapturer* capturer);
    // impl. of MediaTimerCallback
    void onTimeout(uint64_t) final { captureNextFrame(); }
private:
    const std::shared_ptr<webrtc::TaskQueueBase> _timerQueue;
    MediaTimer _timer;
    std::atomic<int32_t> _fps = webrtc::videocapturemodule::kDefaultFrameRate;
};

template <class TCapturer>
template <typename... Args>
inline DesktopSimpleCapturer<TCapturer>::
    DesktopSimpleCapturer(bool window, bool previewMode,
                          webrtc::DesktopCaptureOptions options,
                          std::shared_ptr<webrtc::TaskQueueBase> timerQueue,
                          VideoFrameBufferPool framesPool,
                          Args&&... args)
    : TCapturer(window, previewMode, std::move(options), std::move(framesPool), std::forward<Args>(args)...)
    , _timerQueue(std::move(timerQueue))
    , _timer(_timerQueue)
{
}

template <class TCapturer>
template <typename... Args>
inline DesktopSimpleCapturer<TCapturer>::
    DesktopSimpleCapturer(bool window, bool previewMode,
                          webrtc::DesktopCaptureOptions options,
                          VideoFrameBufferPool framesPool,
                          Args&&... args)
    : DesktopSimpleCapturer(window, previewMode, std::move(options),
                            createTaskQueueS(formatQueueName(this), webrtc::TaskQueueFactory::Priority::HIGH),
                            std::move(framesPool), std::forward<Args>(args)...)
{
}

template <class TCapturer>
inline DesktopSimpleCapturer<TCapturer>::~DesktopSimpleCapturer()
{
    DesktopSimpleCapturer::stop();
}

template <class TCapturer>
inline void DesktopSimpleCapturer<TCapturer>::setTargetFramerate(int32_t fps)
{
    fps = DesktopConfiguration::boundFramerate(fps);
    if (exchangeVal(fps, _fps)) {
        if (_timer.started()) {
            _timer.stop();
            if (canStart()) {
                _timer.startWithFramerate(fps);
            }
        }
    }
}

template <class TCapturer>
inline bool DesktopSimpleCapturer<TCapturer>::start()
{
    if (!started() && canStart()) {
        _timer.setCallback(this);
        _timer.startWithFramerate(_fps);
        return true;
    }
    return false;
}

template <class TCapturer>
inline void DesktopSimpleCapturer<TCapturer>::stop()
{
    _timer.setCallback(nullptr);
    _timer.stop();
}

template <class TCapturer>
inline void DesktopSimpleCapturer<TCapturer>::execute(absl::AnyInvocable<void()&&> task, uint64_t delayMs)
{
    _timer.singleShot(std::move(task), delayMs);
}

template <class TCapturer>
inline std::string DesktopSimpleCapturer<TCapturer>::formatQueueName(const DesktopSimpleCapturer* capturer)
{
    return "sharing_queue#" + toHexValue(reinterpret_cast<uint64_t>(capturer));
}
	
} // namespace LiveKitCpp
