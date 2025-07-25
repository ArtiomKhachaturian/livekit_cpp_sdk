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
#include "MediaTimerImpl.h"
#include "PeerConnectionFactory.h"
#include <cmath>

namespace LiveKitCpp
{

MediaTimer::MediaTimer(const std::shared_ptr<webrtc::TaskQueueBase>& queue,
                       MediaTimerCallback* callback)
    : RtcObject<MediaTimerImpl>(id(), queue)
{
    setCallback(callback);
}

MediaTimer::MediaTimer(const PeerConnectionFactory* pcf,
                       MediaTimerCallback* callback)
    : MediaTimer(pcf ? pcf->eventsQueue() : nullptr, callback)
{
}

MediaTimer::MediaTimer(const webrtc::scoped_refptr<const PeerConnectionFactory>& pcf,
                       MediaTimerCallback* callback)
    : MediaTimer(pcf.get(), callback)
{
}

MediaTimer::~MediaTimer()
{
    if (auto impl = dispose()) {
        impl->setInvalid();
    }
}

bool MediaTimer::started() const noexcept
{
    const auto impl = loadImpl();
    return impl && impl->started();
}

webrtc::TaskQueueBase::DelayPrecision MediaTimer::precision() const
{
    if (const auto impl = loadImpl()) {
        return impl->precision();
    }
    return webrtc::TaskQueueBase::DelayPrecision::kLow;
}

void MediaTimer::setPrecision(webrtc::TaskQueueBase::DelayPrecision precision)
{
    if (const auto impl = loadImpl()) {
        impl->setPrecision(precision);
    }
}

void MediaTimer::setCallback(MediaTimerCallback* callback)
{
    if (const auto impl = loadImpl()) {
        impl->setCallback(callback);
    }
}

void MediaTimer::start(uint64_t intervalMs)
{
    if (const auto impl = loadImpl()) {
        impl->start(intervalMs);
    }
}

void MediaTimer::startWithFramerate(float fps)
{
    if (FP_ZERO != std::fpclassify(fps)) {
        start(uint64_t(std::round(1000ULL / fps)));
    }
}

void MediaTimer::stop()
{
    if (const auto impl = loadImpl()) {
        impl->stop();
    }
}

void MediaTimer::singleShot(absl::AnyInvocable<void()&&> task,
                            uint64_t delayMs, uint64_t id)
{
    if (const auto impl = loadImpl()) {
        impl->singleShot(std::move(task), delayMs, id);
    }
}

void MediaTimer::singleShot(MediaTimerCallback* callback,
                            uint64_t delayMs, uint64_t id)
{
    if (callback) {
        singleShot([timerId = this->id(), callback]() {
            callback->onTimeout(timerId);
        }, delayMs, id);
    }
}

void MediaTimer::singleShot(const std::shared_ptr<MediaTimerCallback>& callback,
                            uint64_t delayMs, uint64_t id)
{
    if (callback) {
        singleShot([timerId = this->id(), callback]() {
            callback->onTimeout(timerId);
        }, delayMs, id);
    }
}

void MediaTimer::singleShot(std::unique_ptr<MediaTimerCallback> callback,
                            uint64_t delayMs, uint64_t id)
{
    if (callback) {
        singleShot([timerId = this->id(), callback = std::move(callback)]() {
            callback->onTimeout(timerId);
        }, delayMs, id);
    }
}

void MediaTimer::cancelSingleShot(uint64_t id)
{
    if (const auto impl = loadImpl()) {
        impl->cancelSingleShot(id);
    }
}

} // namespace LiveKitCpp
