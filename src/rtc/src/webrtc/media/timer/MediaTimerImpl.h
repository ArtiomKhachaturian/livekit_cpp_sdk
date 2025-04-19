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
#pragma once // MediaTimerImpl.h
#include "Listener.h"
#include <api/task_queue/task_queue_base.h>
#include <memory>
#include <unordered_set>


namespace LiveKitCpp
{

class MediaTimerCallback;

class MediaTimerImpl : public std::enable_shared_from_this<MediaTimerImpl>
{
public:
    MediaTimerImpl(uint64_t timerId, const std::shared_ptr<webrtc::TaskQueueBase>& queue);
    ~MediaTimerImpl() { setInvalid(); }
    void setPrecision(webrtc::TaskQueueBase::DelayPrecision precision) { _precision = precision; }
    webrtc::TaskQueueBase::DelayPrecision precision() const { return _precision; }
    void setInvalid();
    void setCallback(MediaTimerCallback* callback) { _callback = callback; }
    bool started() const { return _started; }
    void start(uint64_t intervalMs);
    void stop() { _started = false; }
    void singleShot(absl::AnyInvocable<void()&&> task, uint64_t delayMs = 0ULL, uint64_t id = 0ULL);
    void cancelSingleShot(uint64_t id) { popSingleShot(id); }
private:
    bool valid(bool queueOnly) const noexcept;
    void pushSingleShot(uint64_t id);
    bool popSingleShot(uint64_t id);
    void post(uint64_t intervalMs, bool first);
    void nextTick(uint64_t intervalMs);
private:
    const uint64_t _timerId;
    const std::weak_ptr<webrtc::TaskQueueBase> _queue;
    Bricks::SafeObj<bool> _valid = true;
    Bricks::Listener<MediaTimerCallback*> _callback;
    Bricks::SafeObj<std::unordered_set<uint64_t>> _singleShotIds;
    std::atomic<webrtc::TaskQueueBase::DelayPrecision> _precision = webrtc::TaskQueueBase::DelayPrecision::kLow;
    std::atomic_bool _started = false;
};
	
} // namespace LiveKitCpp
