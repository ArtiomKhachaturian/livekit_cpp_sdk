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
#pragma once // FieldTrials.h
#include <api/field_trials_view.h>
#include <unordered_map>

namespace LiveKitCpp
{

class FieldTrials : public webrtc::FieldTrialsView
{
    using ValuesMap = std::unordered_map<absl::string_view, std::string>;
public:
    FieldTrials() = default;
    void add(absl::string_view key, std::string value);
    void setEnabled(absl::string_view key, bool enabled);
    void remove(absl::string_view key);
    // impl. of webrtc::FieldTrialsView
    std::string Lookup(absl::string_view key) const final;
private:
    ValuesMap _values;
};

} // namespace LiveKitCpp
