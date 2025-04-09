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
#pragma once // StatsReport.h
#include "livekit/LiveKitClientExport.h"
#include "livekit/stats/Stats.h"
#include <chrono>
#include <memory>
#include <vector>

namespace LiveKitCpp
{

struct StatsReportData;

class LIVEKIT_CLIENT_API StatsReport
{
    friend class StatsSourceImpl;
public:
    StatsReport() = default;
    StatsReport(const StatsReport& src) noexcept;
    StatsReport(StatsReport&& tmp) noexcept;
    ~StatsReport();
    StatsReport& operator = (const StatsReport& src) noexcept;
    StatsReport& operator = (StatsReport&& tmp) noexcept;
    // Unix time in milliseconds
    std::chrono::time_point<std::chrono::system_clock> timestamp() const;
    // count of stats
    size_t size() const;
    Stats get(const std::string& id) const;
    Stats get(size_t index) const;
    // Creates a JSON readable string representation of the report,
    // listing all of its stats objects.
    std::string json() const;
    explicit operator bool() const { return size() > 0U; }
private:
    StatsReport(StatsReportData* data) noexcept;
private:
    std::shared_ptr<StatsReportData> _data;
};

} // namespace LiveKitCpp
