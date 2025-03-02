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
#pragma once // MutexTraits.h
#include "SharedLockGuard.h"
#include <mutex>
#include <shared_mutex>

namespace LiveKitCpp
{

template<class TMutexType> struct MutexTraits {
    using WriteLock = std::lock_guard<TMutexType>;
    using ReadLock  = WriteLock;
};

template<> struct MutexTraits<std::shared_mutex> {
    using WriteLock = std::lock_guard<std::shared_mutex>;
    using ReadLock  = SharedLockGuard<std::shared_mutex>;
};


} // namespace LiveKitCpp
