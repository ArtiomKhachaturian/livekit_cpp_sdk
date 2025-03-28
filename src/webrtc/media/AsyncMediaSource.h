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
#pragma once // AsyncMediaSource.h
#include "AsyncMediaSourceImpl.h"
#include "ThreadUtils.h"
#include <api/media_stream_interface.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template<class TMediaSource, class TImpl>
class AsyncMediaSource : public TMediaSource
{
    static_assert(std::is_base_of_v<AsyncMediaSourceImpl, TImpl>);
    static_assert(std::is_base_of_v<webrtc::MediaSourceInterface, TMediaSource>);
public:
    const auto& signalingQueue() const noexcept { return _impl->signalingQueue(); }
    bool enabled() const noexcept { return _enabled; }
    bool setEnabled(bool enabled);
    void close() { _impl->close(); }
    // impl. of MediaSourceInterface
    webrtc::MediaSourceInterface::SourceState state() const final { return _impl->state(); }
    bool remote() const final { return false; }
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
protected:
    template<typename... Args>
    AsyncMediaSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     Args&&... args);
    template <class Method, typename... Args>
    void postToImpl(Method method, Args&&... args) const;
protected:
    const std::shared_ptr<TImpl> _impl;
private:
    std::atomic_bool _enabled = true;
};

template<class TMediaSource, class TImpl>
template<typename... Args>
inline AsyncMediaSource<TMediaSource, TImpl>::
    AsyncMediaSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     Args&&... args)
    : _impl(std::make_shared<TImpl>(std::move(signalingQueue), logger, std::forward<Args>(args)...))
{
}

template<class TMediaSource, class TImpl>
inline bool AsyncMediaSource<TMediaSource, TImpl>::setEnabled(bool enabled)
{
    if (_impl->active() && enabled != _enabled.exchange(enabled)) {
        postToImpl(&TImpl::setEnabled, enabled);
        _impl->notifyAboutChanges();
        return true;
    }
    return false;
}

template<class TMediaSource, class TImpl>
inline void AsyncMediaSource<TMediaSource, TImpl>::RegisterObserver(webrtc::ObserverInterface* observer)
{
    _impl->registerObserver(observer);
}

template<class TMediaSource, class TImpl>
inline void AsyncMediaSource<TMediaSource, TImpl>::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    _impl->unregisterObserver(observer);
}

template<class TMediaSource, class TImpl>
template <class Method, typename... Args>
inline void AsyncMediaSource<TMediaSource, TImpl>::postToImpl(Method method, Args&&... args) const
{
    postOrInvoke(signalingQueue(), _impl, false, method, std::forward<Args>(args)...);
}

} // namespace LiveKitCpp
