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
#include "MediaTimer.h"
#include "Listener.h"
#include <thread>

namespace {

inline constexpr webrtc::TimeDelta toTimeDelta(uint64_t intervalMs) {
    return webrtc::TimeDelta::Millis(intervalMs);
}

}

namespace LiveKitCpp
{

struct MediaTimer::Impl : public std::enable_shared_from_this<Impl>
{
    Impl(MediaTimer* timer, webrtc::TaskQueueBase* queue, MediaTimerCallback* callback);
    webrtc::TaskQueueBase* const _queue;
    MediaTimer* const _timer;
    Bricks::Listener<MediaTimerCallback*> _callback;
    std::atomic<webrtc::TaskQueueBase::DelayPrecision> _precision = webrtc::TaskQueueBase::DelayPrecision::kLow;
    std::atomic_bool _started = false;
    void post(uint64_t intervalMs);
};

MediaTimer::MediaTimer(webrtc::TaskQueueBase* queue, MediaTimerCallback* callback)
    : _impl(std::make_shared<Impl>(this, queue, callback))
{
}

MediaTimer::~MediaTimer()
{
    stop();
    _impl->_callback.reset();
}

webrtc::TaskQueueBase::DelayPrecision MediaTimer::precision() const
{
    return _impl->_precision;
}

void MediaTimer::setPrecision(webrtc::TaskQueueBase::DelayPrecision precision)
{
    _impl->_precision = precision;
}

void MediaTimer::start(uint64_t intervalMs)
{
    if (_impl->_queue && _impl->_callback && !_impl->_started.exchange(true)) {
        _impl->post(intervalMs);
    }
}

void MediaTimer::stop()
{
    _impl->_started = false;
}

MediaTimer::Impl::Impl(MediaTimer* timer, webrtc::TaskQueueBase* queue, MediaTimerCallback* callback)
    : _queue(queue)
    , _timer(timer)
    , _callback(callback)
{
}

void MediaTimer::Impl::post(uint64_t intervalMs)
{
    _queue->PostDelayedTaskWithPrecision(_precision, [intervalMs, weak = weak_from_this()]() {
        const auto self = weak.lock();
        if (self && self->_started) {
            self->_callback.invoke(&MediaTimerCallback::onTimeout, self->_timer);
            self->post(intervalMs);
        }
    }, toTimeDelta(intervalMs));
}

} // namespace LiveKitCpp
