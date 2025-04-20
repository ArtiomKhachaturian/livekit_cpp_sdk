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
#include "Utils.h"
#include "ThreadUtils.h"
#include <api/media_stream_interface.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template <class TMediaSource, class TAsyncImpl>
class AsyncMediaSource : public TMediaSource
{
    static_assert(std::is_base_of_v<AsyncMediaSourceImpl, TAsyncImpl>);
    static_assert(std::is_base_of_v<webrtc::MediaSourceInterface, TMediaSource>);
public:
    ~AsyncMediaSource() override { close(); }
    bool active() const noexcept { return _impl && _impl->active(); }
    const auto& signalingQueue() const noexcept { return _impl->signalingQueue(); }
    bool enabled() const noexcept { return _enabled; }
    bool setEnabled(bool enabled);
    void close();
    // impl. of MediaSourceInterface
    webrtc::MediaSourceInterface::SourceState state() const final { return _impl->state(); }
    bool remote() const final { return false; }
    // impl. of NotifierInterface
    void RegisterObserver(webrtc::ObserverInterface* observer) final;
    void UnregisterObserver(webrtc::ObserverInterface* observer) final;
protected:
    template <typename... Args>
    AsyncMediaSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     Args&&... args);
    AsyncMediaSource(std::shared_ptr<TAsyncImpl> impl);
    template <class Method, typename... Args>
    void postToImpl(Method method, Args&&... args) const;
protected:
    const std::shared_ptr<TAsyncImpl> _impl;
private:
    std::atomic_bool _enabled = true;
};

template <class TMediaSource, class TAsyncImpl>
template <typename... Args>
inline AsyncMediaSource<TMediaSource, TAsyncImpl>::
    AsyncMediaSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                     const std::shared_ptr<Bricks::Logger>& logger,
                     Args&&... args)
    : AsyncMediaSource(std::make_shared<TAsyncImpl>(std::move(signalingQueue), logger,
                                                    std::forward<Args>(args)...))
{
}

template <class TMediaSource, class TAsyncImpl>
inline AsyncMediaSource<TMediaSource, TAsyncImpl>::AsyncMediaSource(std::shared_ptr<TAsyncImpl> impl)
    : _impl(std::move(impl))
{
}

template <class TMediaSource, class TAsyncImpl>
inline bool AsyncMediaSource<TMediaSource, TAsyncImpl>::setEnabled(bool enabled)
{
    if (_impl && _impl->active() && exchangeVal(enabled, _enabled)) {
        postToImpl(&TAsyncImpl::setEnabled, enabled);
        _impl->notifyAboutChanges();
        return true;
    }
    return false;
}

template <class TMediaSource, class TAsyncImpl>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::close()
{
    if (_impl && _impl->active()) {
        postToImpl(&TAsyncImpl::close);
    }
}

template <class TMediaSource, class TAsyncImpl>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::RegisterObserver(webrtc::ObserverInterface* observer)
{
    if (_impl && _impl->active()) {
        _impl->registerObserver(observer);
    }
}

template <class TMediaSource, class TAsyncImpl>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    if (_impl) {
        _impl->unregisterObserver(observer);
    }
}

template <class TMediaSource, class TAsyncImpl>
template <class Method, typename... Args>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::
    postToImpl(Method method, Args&&... args) const
{
    if (_impl) {
        postOrInvokeW(signalingQueue(), _impl, false, method, std::forward<Args>(args)...);
    }
}

} // namespace LiveKitCpp
