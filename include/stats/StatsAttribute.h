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
#pragma once // StatsAttribute.h
#include "LiveKitClientExport.h"
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace LiveKitCpp
{

class LIVEKIT_CLIENT_API StatsAttribute
{
    using Value = std::variant<const std::optional<bool>*,
                               const std::optional<int32_t>*,
                               const std::optional<uint32_t>*,
                               const std::optional<int64_t>*,
                               const std::optional<uint64_t>*,
                               const std::optional<double>*,
                               const std::optional<std::string>*,
                               const std::optional<std::vector<bool>>*,
                               const std::optional<std::vector<int32_t>>*,
                               const std::optional<std::vector<uint32_t>>*,
                               const std::optional<std::vector<int64_t>>*,
                               const std::optional<std::vector<uint64_t>>*,
                               const std::optional<std::vector<double>>*,
                               const std::optional<std::vector<std::string>>*,
                               const std::optional<std::map<std::string, uint64_t>>*,
                               const std::optional<std::map<std::string, double>>*>;
    friend class Stats;
public:
    StatsAttribute() = default;
    std::string_view name() const;
    bool valid() const;
    explicit operator bool() const { return valid(); }
    template <typename T>
    bool isType() const { return std::holds_alternative<const std::optional<T>*>(_value); }
    template <typename T>
    const std::optional<T>& optional() const { return *std::get<const std::optional<T>*>(_value); }
    template <typename T>
    const T& value() const { return optional<T>().value(); };
    // is vector or not
    bool isSequence() const;
    // is map or not
    bool isAssociative() const;
    std::string toString() const;
    bool operator==(const StatsAttribute& other) const;
    bool operator!=(const StatsAttribute& other) const;
private:
    StatsAttribute(std::string_view name, Value value);
private:
    std::string_view _name;
    Value _value;
};

} // namespace LiveKitCpp
