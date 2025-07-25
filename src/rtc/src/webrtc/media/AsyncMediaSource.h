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
#include "RtcObject.h"
#include "Utils.h"
#include "ThreadUtils.h"
#include <api/media_stream_interface.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template <class TMediaSource, class TAsyncImpl>
class AsyncMediaSource : public RtcObject<TAsyncImpl, TMediaSource>
{
    static_assert(std::is_base_of_v<AsyncMediaSourceImpl, TAsyncImpl>);
    static_assert(std::is_base_of_v<webrtc::MediaSourceInterface, TMediaSource>);
    using Base = RtcObject<TAsyncImpl, TMediaSource>;
public:
    ~AsyncMediaSource() override;
    bool active() const noexcept;
    std::weak_ptr<webrtc::TaskQueueBase> signalingQueue() const noexcept;
    bool enabled() const noexcept;
    bool setEnabled(bool enabled);
    void close();
    // impl. of MediaSourceInterface
    webrtc::MediaSourceInterface::SourceState state() const final;
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
    : Base(std::move(impl))
{
}

template <class TMediaSource, class TAsyncImpl>
inline AsyncMediaSource<TMediaSource, TAsyncImpl>::~AsyncMediaSource()
{
    if (auto impl = Base::dispose()) {
        const auto q = impl->signalingQueue();
        postTask(q, [impl = std::move(impl)]() mutable {
            if (impl->active()) {
                impl->close();
            }
            impl.reset();
        });
    }
}

template <class TMediaSource, class TAsyncImpl>
inline bool AsyncMediaSource<TMediaSource, TAsyncImpl>::active() const noexcept
{
    if (const auto impl = Base::loadImpl()) {
        return impl->active();
    }
    return false;
}

template <class TMediaSource, class TAsyncImpl>
inline std::weak_ptr<webrtc::TaskQueueBase> AsyncMediaSource<TMediaSource, TAsyncImpl>::signalingQueue() const noexcept
{
    if (const auto impl = Base::loadImpl()) {
        return impl->signalingQueue();
    }
    return {};
}

template <class TMediaSource, class TAsyncImpl>
inline bool AsyncMediaSource<TMediaSource, TAsyncImpl>::enabled() const noexcept
{
    if (const auto impl = Base::loadImpl()) {
        return impl->enabled();
    }
    return false;
}

template <class TMediaSource, class TAsyncImpl>
inline bool AsyncMediaSource<TMediaSource, TAsyncImpl>::setEnabled(bool enabled)
{
    const auto impl = Base::loadImpl();
    if (impl && impl->setEnabled(enabled)) {
        postToImpl(&TAsyncImpl::updateAfterEnableChanges, enabled);
        return true;
    }
    return false;
}

template <class TMediaSource, class TAsyncImpl>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::close()
{
    const auto impl = Base::loadImpl();
    if (impl && impl->active()) {
        postToImpl(&TAsyncImpl::close);
    }
}

template <class TMediaSource, class TAsyncImpl>
inline webrtc::MediaSourceInterface::SourceState AsyncMediaSource<TMediaSource, TAsyncImpl>::state() const
{
    if (const auto impl = Base::loadImpl()) {
        return impl->state();
    }
    return webrtc::MediaSourceInterface::SourceState::kEnded;
}

template <class TMediaSource, class TAsyncImpl>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::RegisterObserver(webrtc::ObserverInterface* observer)
{
    const auto impl = Base::loadImpl();
    if (impl && impl->active()) {
        impl->registerObserver(observer);
    }
}

template <class TMediaSource, class TAsyncImpl>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::UnregisterObserver(webrtc::ObserverInterface* observer)
{
    if (const auto impl = Base::loadImpl()) {
        impl->unregisterObserver(observer);
    }
}

template <class TMediaSource, class TAsyncImpl>
template <class Method, typename... Args>
inline void AsyncMediaSource<TMediaSource, TAsyncImpl>::
    postToImpl(Method method, Args&&... args) const
{
    if (const auto impl = Base::loadImpl()) {
        postOrInvokeW<false>(signalingQueue(), impl, method, std::forward<Args>(args)...);
    }
}

} // namespace LiveKitCpp
