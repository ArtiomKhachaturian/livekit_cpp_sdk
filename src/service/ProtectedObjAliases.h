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
#pragma once // ProtectedObjAliases.h
#include "ProtectedObj.h"
#include <memory>
#include <optional>

namespace LiveKitCpp
{
// aliases for most common std types

template <typename T, class TMutexType = std::recursive_mutex>
using ProtectedSharedPtr = ProtectedObj<std::shared_ptr<T>, TMutexType>;

template <typename T, class TMutexType = std::recursive_mutex>
using ProtectedWeakPtr = ProtectedObj<std::weak_ptr<T>, TMutexType>;

template <typename T, class TMutexType = std::recursive_mutex>
using ProtectedUniquePtr = ProtectedObj<std::unique_ptr<T>, TMutexType>;

template <typename T, class TMutexType = std::recursive_mutex>
using ProtectedOptional = ProtectedObj<std::optional<T>, TMutexType>;

} // namespace LiveKitCpp
