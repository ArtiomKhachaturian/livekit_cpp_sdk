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
#include "MediaTimerImpl.h"
#include "MediaTimerCallback.h"
#include "Utils.h"

namespace {

inline constexpr webrtc::TimeDelta toTimeDelta(uint64_t intervalMs) {
    return webrtc::TimeDelta::Millis(intervalMs);
}

}

namespace LiveKitCpp
{

MediaTimerImpl::MediaTimerImpl(uint64_t timerId, const std::shared_ptr<webrtc::TaskQueueBase>& queue)
    : _timerId(timerId)
    , _queue(queue)
{
}

void MediaTimerImpl::setInvalid()
{
    if (exchangeVal(false, _valid)) {
        stop();
        setCallback(nullptr);
        _singleShotIds({});
    }
}

void MediaTimerImpl::start(uint64_t intervalMs)
{
    if (!_queue.expired() && !_started.exchange(true)) {
        post(intervalMs, true);
    }
}

void MediaTimerImpl::singleShot(absl::AnyInvocable<void()&&> task, uint64_t delayMs, uint64_t id)
{
    if (const auto q = _queue.lock()) {
        pushSingleShot(id);
        auto fn = [id, task = std::move(task), weak = weak_from_this()]() mutable {
            const auto self = weak.lock();
            if (self && self->valid(false) && self->popSingleShot(id)) {
                std::move(task)();
            }
        };
        q->PostDelayedTaskWithPrecision(_precision, std::move(fn), toTimeDelta(delayMs));
    }
}

bool MediaTimerImpl::valid(bool queueOnly) const noexcept
{
    return !_queue.expired() && (queueOnly || _valid());
}

void MediaTimerImpl::pushSingleShot(uint64_t id)
{
    if (id) {
        LOCK_WRITE_SAFE_OBJ(_singleShotIds);
        _singleShotIds->insert(id);
    }
}

bool MediaTimerImpl::popSingleShot(uint64_t id)
{
    if (id) {
        LOCK_WRITE_SAFE_OBJ(_singleShotIds);
        return _singleShotIds->erase(id) > 0U;
    }
    return true;
}

void MediaTimerImpl::post(uint64_t intervalMs, bool first)
{
    const auto q = _queue.lock();
    if (q && started()) {
        q->PostDelayedTaskWithPrecision(_precision, [intervalMs, weak = weak_from_this()]() {
            if (const auto self = weak.lock()) {
                self->nextTick(intervalMs);
            }
        }, toTimeDelta(first ? 0LL : intervalMs));
    }
}

void MediaTimerImpl::nextTick(uint64_t intervalMs)
{
    if (valid(true) && _started) {
        bool next = false;
        {
            LOCK_READ_SAFE_OBJ(_valid);
            if (_valid.constRef()) {
                _callback.invoke(&MediaTimerCallback::onTimeout, _timerId);
                next = true;
            }
        }
        if (next) {
            post(intervalMs, false);
        }
    }
}

} // namespace LiveKitCpp
