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
#pragma once // SharedLockGuard.h
#include <mutex>

namespace LiveKitCpp
{

template<class TSharedMutex>
class SharedLockGuard
{
public:
    SharedLockGuard(TSharedMutex& mutex);
    SharedLockGuard(TSharedMutex& mutex, std::adopt_lock_t);
    ~SharedLockGuard() { _mutex.unlock_shared(); }
private:
    SharedLockGuard(const SharedLockGuard&) = delete;
    SharedLockGuard& operator = (const SharedLockGuard&) = delete;
private:
    TSharedMutex& _mutex;
};

template<class TSharedMutex>
SharedLockGuard<TSharedMutex>::SharedLockGuard(TSharedMutex& mutex)
    : _mutex(mutex)
{
    _mutex.lock_shared();
}

template<class TSharedMutex>
SharedLockGuard<TSharedMutex>::SharedLockGuard(TSharedMutex& mutex, std::adopt_lock_t)
    : _mutex(mutex)
{
}

} // namespace LiveKitCpp
