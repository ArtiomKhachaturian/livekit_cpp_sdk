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
#pragma once // ThreadUtils.h
#include "Invoke.h"
#include <rtc_base/thread.h>
#include <memory>
#include <tuple>
#include <type_traits>

namespace LiveKitCpp
{

inline void invokeInThread(webrtc::Thread* to,
                           rtc::FunctionView<void()> handler) {
    if (to && !to->IsQuitting()) {
        if (to->IsCurrent()) {
            std::move(handler)();
        }
        else {
            to->BlockingCall(std::move(handler));
        }
    }
}

template <typename Handler, typename R = std::invoke_result_t<Handler>>
inline R invokeInThreadR(webrtc::Thread* to, Handler handler, R defaultVal = {})
{
    if (to && !to->IsQuitting()) {
        if (to->IsCurrent()) {
            return std::move(handler)();
        }
        return to->BlockingCall<Handler, R>(std::move(handler));
    }
    return defaultVal;
}

inline void postTask(webrtc::TaskQueueBase* queue, absl::AnyInvocable<void()&&> task)
{
    if (queue) {
        queue->PostTask(std::move(task));
    }
}

inline void postTask(const std::weak_ptr<webrtc::TaskQueueBase>& queue,
                     absl::AnyInvocable<void()&&> task)
{
    postTask(queue.lock().get(), std::move(task));
}

template <class TListener, class Method, typename... Args>
inline void postOrInvoke(webrtc::TaskQueueBase* queue,
                         const std::shared_ptr<TListener>& listener,
                         bool forcePost, Method method, Args&&... args)
{
    if (queue && listener) {
        using Invoker = Bricks::Invoke<std::shared_ptr<TListener>>;
        if (forcePost || !queue->IsCurrent()) {
            using WeakRef = std::weak_ptr<TListener>;
            queue->PostTask([method = std::move(method), // deep copy of all arguments
                             args = std::make_tuple((typename std::decay<Args>::type)args...),
                             weak = WeakRef(listener)](){
                if (const auto strong = weak.lock()) {
                    std::apply([&strong, &method](auto&&... args) {
                        Invoker::make(strong, method, std::forward<decltype(args)>(args)...);
                    }, args);
                }
            });
        }
        else {
            Invoker::make(listener, method, std::forward<Args>(args)...);
        }
    }
}

template <class TListener, class Method, typename... Args>
inline void postOrInvokeS(const std::shared_ptr<webrtc::TaskQueueBase>& queue,
                          const std::shared_ptr<TListener>& listener,
                          bool forcePost, Method method, Args&&... args)
{
    postOrInvoke(queue.get(), listener, forcePost, std::move(method), std::forward<Args>(args)...);
}

template <class TListener, class Method, typename... Args>
inline void postOrInvokeW(const std::weak_ptr<webrtc::TaskQueueBase>& queue,
                          const std::shared_ptr<TListener>& listener,
                          bool forcePost, Method method, Args&&... args)
{
    if (listener) {
        postOrInvokeS(queue.lock(), listener, forcePost, std::move(method), std::forward<Args>(args)...);
    }
}

} // namespace LiveKitCpp
