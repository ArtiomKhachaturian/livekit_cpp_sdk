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
#pragma once // MediaTimer.h
#include "Loggable.h"
#include "MediaTimerCallback.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <api/task_queue/task_queue_base.h>
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class PeerConnectionFactory;

class MediaTimer : public Bricks::LoggableS<>
{
    struct Impl;
public:
    // Note: MediaTimer keeps only weak reference to [queue]
    MediaTimer(const std::shared_ptr<webrtc::TaskQueueBase>& queue,
               MediaTimerCallback* callback,
               const std::shared_ptr<Bricks::Logger>& logger = {},
               std::string timerName = {});
    MediaTimer(const PeerConnectionFactory* pcf, MediaTimerCallback* callback,
               const std::shared_ptr<Bricks::Logger>& logger = {},
               std::string timerName = {});
    MediaTimer(const webrtc::scoped_refptr<const PeerConnectionFactory>& pcf,
               MediaTimerCallback* callback,
               const std::shared_ptr<Bricks::Logger>& logger = {},
               std::string timerName = {});
    ~MediaTimer();
    // means that timer's queue is valid, default callback (which already passed via constructor)
    // maybe NULL but [singleShot] methods is available for work
    bool valid() const noexcept;
    // Low by default
    webrtc::TaskQueueBase::DelayPrecision precision() const;
    void setPrecision(webrtc::TaskQueueBase::DelayPrecision precision);
    void setLowPrecision() { setPrecision(webrtc::TaskQueueBase::DelayPrecision::kLow); }
    void setHighPrecision() { setPrecision(webrtc::TaskQueueBase::DelayPrecision::kHigh); }
    void start(uint64_t intervalMs);
    void stop();
    void singleShot(absl::AnyInvocable<void()&&> task, uint64_t delayMs = 0ULL);
    [[deprecated("use single shot methods with smart pointers for better safety")]]
    void singleShot(MediaTimerCallback* callback, uint64_t delayMs = 0ULL);
    void singleShot(const std::shared_ptr<MediaTimerCallback>& callback, uint64_t delayMs = 0ULL);
    void singleShot(std::unique_ptr<MediaTimerCallback> callback, uint64_t delayMs = 0ULL);
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    const std::shared_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
