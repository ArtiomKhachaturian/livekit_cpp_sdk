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
#pragma once // ProtectedObj.h
#include "MutexTraits.h"

// Do magic! Creates a unique name using the line number
#define PROTECTED_OBJ_NAME(prefix) PROTECTED_OBJ_JOIN(prefix, __LINE__)
#define PROTECTED_OBJ_JOIN(symbol1, symbol2) DO_PROTECTED_OBJ_JOIN(symbol1, symbol2)
#define DO_PROTECTED_OBJ_JOIN(symbol1, symbol2) symbol1##symbol2
#define LOCK_READ_PROTECTED_OBJ(object) const decltype(object)::ReadLock PROTECTED_OBJ_NAME(rl)(object.mutex())
#define LOCK_WRITE_PROTECTED_OBJ(object) const decltype(object)::WriteLock PROTECTED_OBJ_NAME(wl)(object.mutex())

// Performance tests for different kind of mutexes:
//
// 1 million iterations, release type build, single thread
// std::mutex           - LOCK_READ_PROTECTED_OBJ 19-21 ms  LOCK_WRITE_PROTECTED_OBJ 19-21 ms
// std::recursive_mutex - LOCK_READ_PROTECTED_OBJ 32-33 ms, LOCK_WRITE_PROTECTED_OBJ 32-33 ms
// std::shared_mutex    - LOCK_READ_PROTECTED_OBJ 41 ms,    LOCK_WRITE_PROTECTED_OBJ 43 ms

namespace LiveKitCpp
{

template <typename T,
class TMutexType = std::recursive_mutex,
class TMutexTraits = MutexTraits<TMutexType>>
class ProtectedObj
{
public:
    using Traits    = TMutexTraits;
    using WriteLock = typename Traits::WriteLock;
    using ReadLock  = typename Traits::ReadLock;
public:
    ProtectedObj() = default;
    explicit ProtectedObj(T val);
    explicit ProtectedObj(ProtectedObj&& tmp) noexcept;
    template <class... Args>
    ProtectedObj(Args&&... args);
    ProtectedObj(const ProtectedObj&) = delete;
    ProtectedObj& operator = (const ProtectedObj&) = delete;
    ProtectedObj& operator = (ProtectedObj&& tmp) noexcept;
    template <typename U = T>
    ProtectedObj& operator = (U src) noexcept;
    auto& mutex() const noexcept { return _mtx; }
    operator const T&() const noexcept { return constRef(); }
    operator T&() noexcept { return ref(); }
    const T& constRef() const noexcept { return _obj; }
    T& ref() noexcept { return _obj; }
    T take() noexcept { return std::move(_obj); }
    T exchange(T obj) {
        std::swap(_obj, obj);
        return obj; // old
    }
    T* operator -> () noexcept { return &_obj; }
    const T* operator -> () const noexcept { return &_obj; }
private:
    mutable TMutexType _mtx;
    T _obj;
};

// impl. of ProtectedObj
template <typename T, class TMutexType, class TMutexTraits>
inline ProtectedObj<T, TMutexType, TMutexTraits>::ProtectedObj(T val)
    : _obj(std::move(val))
{
}

template <typename T, class TMutexType, class TMutexTraits>
inline ProtectedObj<T, TMutexType, TMutexTraits>::ProtectedObj(ProtectedObj&& tmp) noexcept
    : _obj(std::move(tmp._obj))
{
}

template <typename T, class TMutexType, class TMutexTraits>
template <class... Args>
inline ProtectedObj<T, TMutexType, TMutexTraits>::ProtectedObj(Args&&... args)
    : _obj(std::forward<Args>(args)...)
{
}

template <typename T, class TMutexType, class TMutexTraits>
inline ProtectedObj<T, TMutexType, TMutexTraits>& ProtectedObj<T, TMutexType, TMutexTraits>::
    operator = (ProtectedObj&& tmp) noexcept
{
    if (&tmp != this) {
        _obj = std::move(tmp._obj);
    }
    return *this;
}

template <typename T, class TMutexType, class TMutexTraits>
template <typename U>
inline ProtectedObj<T, TMutexType, TMutexTraits>& ProtectedObj<T, TMutexType, TMutexTraits>::
    operator = (U src) noexcept
{
    _obj = std::move(src);
    return *this;
}

} // namespace LiveKitCpp
