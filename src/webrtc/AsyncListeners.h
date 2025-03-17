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
#pragma once // AsyncListeners.h
#include "Listeners.h"
#include <rtc_base/thread.h>
#include <memory>
#include <type_traits>

namespace LiveKitCpp
{

template<class TListener>
class AsyncListeners
{
    using Listeners = Bricks::Listeners<TListener, true>;
public:
    AsyncListeners(const std::weak_ptr<rtc::Thread>& thread);
    AsyncListeners(AsyncListeners&&) = delete;
    AsyncListeners(const AsyncListeners&) = delete;
    ~AsyncListeners() { clear(); }
    const auto& thread() const noexcept { return _thread; }
    bool async() const noexcept { return !_thread.expired(); }
    Bricks::AddResult add(const TListener& listener);
    Bricks::RemoveResult remove(const TListener& listener);
    void clear() { _listeners->clear(); }
    bool empty() const noexcept { return _listeners->empty(); }
    size_t size() const noexcept { return _listeners->size(); }
    template <class Method, typename... Args>
    void invoke(Method method, Args&&... args) const;
    AsyncListeners& operator = (const AsyncListeners&) = delete;
    AsyncListeners& operator = (AsyncListeners&&) noexcept = delete;
    template <class Method, typename... Args>
    static void postOrInvoke(const std::weak_ptr<rtc::Thread>& thread,
                             const std::shared_ptr<TListener>& listener,
                             Method method, Args&&... args);
    template <class Method, typename... Args>
    static void postOrInvoke(const std::shared_ptr<rtc::Thread>& thread,
                             const std::shared_ptr<TListener>& listener,
                             Method method, Args&&... args);
private:
    const std::weak_ptr<rtc::Thread> _thread;
    const std::shared_ptr<Listeners> _listeners;
};

template<class TListener>
inline AsyncListeners<TListener>::AsyncListeners(const std::weak_ptr<rtc::Thread>& thread)
    : _thread(thread)
    , _listeners(std::make_shared<Listeners>())
{
}

template<class TListener>
inline Bricks::AddResult AsyncListeners<TListener>::add(const TListener& listener)
{
    return _listeners->add(listener);
}

template<class TListener>
inline Bricks::RemoveResult AsyncListeners<TListener>::remove(const TListener& listener)
{
    return _listeners->remove(listener);
}

template<class TListener>
template <class Method, typename... Args>
inline void AsyncListeners<TListener>::invoke(Method method, Args&&... args) const
{
    if (const auto thread = _thread.lock()) {
        if (!thread->IsCurrent()) {
            using WeakRef = std::weak_ptr<Listeners>;
            thread->PostTask([method = std::move(method), // deep copy of all arguments
                              args = std::make_tuple((typename std::decay<Args>::type)args...),
                              weak = WeakRef(_listeners)](){
                if (const auto strong = weak.lock()) {
                    std::apply([&strong, &method](auto&&... args) {
                        strong->invoke(method, std::forward<decltype(args)>(args)...);
                    }, args);
                }
            });
        }
        else {
            _listeners->invoke(method, std::forward<Args>(args)...);
        }
    }
}

template<class TListener>
template <class Method, typename... Args>
inline void AsyncListeners<TListener>::postOrInvoke(const std::weak_ptr<rtc::Thread>& thread,
                                                    const std::shared_ptr<TListener>& listener,
                                                    Method method, Args&&... args)
{
    if (listener) {
        postOrInvoke(thread.lock(), listener, std::move(method), std::forward<Args>(args)...);
    }
}

template<class TListener>
template <class Method, typename... Args>
inline void AsyncListeners<TListener>::postOrInvoke(const std::shared_ptr<rtc::Thread>& thread,
                                                    const std::shared_ptr<TListener>& listener,
                                                    Method method, Args&&... args)
{
    if (thread && listener) {
        using Invoker = Bricks::Invoke<std::shared_ptr<TListener>>;
        if (!thread->IsCurrent()) {
            using WeakRef = std::weak_ptr<TListener>;
            thread->PostTask([method = std::move(method), // deep copy of all arguments
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

} // namespace LiveKitCpp
