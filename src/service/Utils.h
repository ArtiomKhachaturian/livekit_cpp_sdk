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
#pragma once // Utils.h
#include "MemoryBlock.h"
#include <memory>
#include <optional>
#include <string>

namespace LiveKitCpp
{

std::string operatingSystemVersion();
std::string operatingSystemName();
std::string modelIdentifier();

template <typename TProto>
inline std::optional<TProto> protofromBytes(const void* data, size_t dataLen) {
    if (data && dataLen) {
        TProto instance;
        if (instance.ParseFromArray(data, int(dataLen))) {
            return instance;
        }
    }
    return std::nullopt;
}

template <typename TProto>
inline std::optional<TProto> protofromBytes(const std::shared_ptr<const MemoryBlock>& bytes) {
    if (bytes) {
        return protofromBytes<TProto>(bytes->data(), bytes->size());
    }
    return {};
}

} // namespace LiveKitCpp
