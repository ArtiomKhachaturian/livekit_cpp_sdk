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
#include "FieldTrials.h"

namespace
{

inline std::string boolValue(bool enabled) { return enabled ? "Enabled" : "Disabled"; }

}

namespace LiveKitCpp
{

std::string FieldTrials::Lookup(absl::string_view key) const
{
    const auto it = _values.find(key);
    if (it != _values.end()) {
        return it->second;
    }
    return {};
}

void FieldTrials::add(absl::string_view key, std::string value)
{
    if (!key.empty()) {
        _values[std::move(key)] = std::move(value);
    }
}

void FieldTrials::setEnabled(absl::string_view key, bool enabled)
{
    add(std::move(key), boolValue(enabled));
}

void FieldTrials::remove(absl::string_view key)
{
    if (!key.empty()) {
        _values.erase(key);
    }
}

} // namespace LiveKitCpp
