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
#include "PeerConnectionFactory.h"
#include "Utils.h"
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
    Impl(std::string timerName, MediaTimer* timer,
         const std::shared_ptr<webrtc::TaskQueueBase>& queue,
         MediaTimerCallback* callback);
    const std::string _timerName;
    const std::weak_ptr<webrtc::TaskQueueBase> _queue;
    MediaTimer* const _timer;
    Bricks::Listener<MediaTimerCallback*> _callback;
    std::atomic<webrtc::TaskQueueBase::DelayPrecision> _precision = webrtc::TaskQueueBase::DelayPrecision::kLow;
    std::atomic_bool _started = false;
    bool valid() const noexcept { return !_callback.empty(); }
    void post(uint64_t intervalMs);
};

MediaTimer::MediaTimer(const std::shared_ptr<webrtc::TaskQueueBase>& queue,
                       MediaTimerCallback* callback,
                       const std::shared_ptr<Bricks::Logger>& logger,
                       std::string timerName)
    : Bricks::LoggableS<>(logger)
    , _impl(std::make_shared<Impl>(std::move(timerName), this, queue, callback))
{
}

MediaTimer::MediaTimer(const PeerConnectionFactory* pcf, MediaTimerCallback* callback,
                       const std::shared_ptr<Bricks::Logger>& logger,
                       std::string timerName)
    : MediaTimer(pcf ? pcf->timersQueue() : nullptr, callback, logger, std::move(timerName))
{
}

MediaTimer::MediaTimer(const webrtc::scoped_refptr<const PeerConnectionFactory>& pcf,
                       MediaTimerCallback* callback,
                       const std::shared_ptr<Bricks::Logger>& logger,
                       std::string timerName)
    : MediaTimer(pcf.get(), callback, logger, std::move(timerName))
{
}

MediaTimer::~MediaTimer()
{
    stop();
    _impl->_callback.reset();
}

bool MediaTimer::valid() const noexcept
{
    return !_impl->_queue.expired();
}

webrtc::TaskQueueBase::DelayPrecision MediaTimer::precision() const
{
    return _impl->_precision;
}

void MediaTimer::setPrecision(webrtc::TaskQueueBase::DelayPrecision precision)
{
    const auto prev = _impl->_precision.exchange(precision);
    if (prev != precision) {
        logVerbose(makeStateChangesString(prev, precision));
    }
}

void MediaTimer::start(uint64_t intervalMs)
{
    if ( _impl->_callback && valid() && !_impl->_started.exchange(true)) {
        logVerbose("start media timer with interval " + std::to_string(intervalMs) + " ms");
        _impl->post(intervalMs);
    }
}

void MediaTimer::stop()
{
    if (_impl->_started.exchange(false)) {
        logVerbose("media timer has been stopped");
    }
}

void MediaTimer::singleShot(absl::AnyInvocable<void()&&> task, uint64_t delayMs)
{
    if (const auto q = _impl->_queue.lock()) {
        q->PostDelayedTaskWithPrecision(precision(), std::move(task), toTimeDelta(delayMs));
    }
}

void MediaTimer::singleShot(MediaTimerCallback* callback, uint64_t delayMs)
{
    if (callback && valid()) {
        singleShot([callback, weak = _impl->weak_from_this()]() {
            const auto self = weak.lock();
            if (self && self->valid()) {
                callback->onTimeout(self->_timer);
            }
        }, delayMs);
    }
}

void MediaTimer::singleShot(const std::shared_ptr<MediaTimerCallback>& callback, uint64_t delayMs)
{
    if (callback && valid()) {
        singleShot([callback, weak = _impl->weak_from_this()]() {
            const auto self = weak.lock();
            if (self && self->valid()) {
                callback->onTimeout(self->_timer);
            }
        }, delayMs);
    }
}

void MediaTimer::singleShot(std::unique_ptr<MediaTimerCallback> callback, uint64_t delayMs)
{
    if (callback && valid()) {
        singleShot([callback = std::move(callback), weak = _impl->weak_from_this()]() {
            const auto self = weak.lock();
            if (self && self->valid()) {
                callback->onTimeout(self->_timer);
            }
        }, delayMs);
    }
}

std::string_view MediaTimer::logCategory() const
{
    if (_impl->_timerName.empty()) {
        static const std::string_view category("media_timer");
        return category;
    }
    return _impl->_timerName;
}

MediaTimer::Impl::Impl(std::string timerName, MediaTimer* timer,
                       const std::shared_ptr<webrtc::TaskQueueBase>& queue,
                       MediaTimerCallback* callback)
    : _timerName(std::move(timerName))
    , _queue(queue)
    , _timer(timer)
    , _callback(callback)
{
}

void MediaTimer::Impl::post(uint64_t intervalMs)
{
    if (const auto q = _queue.lock()) {
        q->PostDelayedTaskWithPrecision(_precision, [intervalMs, weak = weak_from_this()]() {
            const auto self = weak.lock();
            if (self && self->_started) {
                self->_callback.invoke(&MediaTimerCallback::onTimeout, self->_timer);
                self->post(intervalMs);
            }
        }, toTimeDelta(intervalMs));
    }
}

} // namespace LiveKitCpp
