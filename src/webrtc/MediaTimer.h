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
#include "MediaTimerCallback.h"
#include <api/media_types.h>
#include <api/task_queue/task_queue_base.h>
#include <atomic>
#include <memory>

namespace LiveKitCpp
{

class MediaTimer
{
    struct Impl;
public:
    MediaTimer(webrtc::TaskQueueBase* queue, MediaTimerCallback* callback);
    ~MediaTimer();
    // Low by default
    webrtc::TaskQueueBase::DelayPrecision precision() const;
    void setPrecision(webrtc::TaskQueueBase::DelayPrecision precision);
    void setLowPrecision() { setPrecision(webrtc::TaskQueueBase::DelayPrecision::kLow); }
    void setHighPrecision() { setPrecision(webrtc::TaskQueueBase::DelayPrecision::kHigh); }
    void start(uint64_t intervalMs);
    void stop();
private:
    const std::shared_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
