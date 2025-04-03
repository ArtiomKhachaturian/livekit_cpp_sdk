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
#pragma once // Stats.h
#include "LiveKitClientExport.h"
#include "StatsAttribute.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace LiveKitCpp
{

struct StatsReportData;

class LIVEKIT_CLIENT_API Stats
{
    friend class StatsReport;
public:
    Stats() = default;
    Stats(const Stats& src) noexcept;
    Stats(Stats&& tmp) noexcept;
    ~Stats();
    Stats& operator = (const Stats& src) noexcept;
    Stats& operator = (Stats&& tmp) noexcept;
    bool valid() const noexcept;
    explicit operator bool() const noexcept { return valid(); }
    const std::string& id() const;
    // Unix time in milliseconds
    std::chrono::time_point<std::chrono::system_clock> timestamp() const;
    std::string_view type() const;
    // Creates a JSON readable string representation of the stats
    // object, listing all of its attributes (names and values).
    std::string json() const;
    // Returns all attributes of this stats object, i.e. a list of its individual
    // metrics as viewed via the Attribute wrapper.
    std::vector<StatsAttribute> attributes() const;
private:
    Stats(const void* stats, const std::shared_ptr<StatsReportData>& data);
private:
    const void* _stats = nullptr;
    std::shared_ptr<StatsReportData> _data;
};

} // namespace LiveKitCpp
