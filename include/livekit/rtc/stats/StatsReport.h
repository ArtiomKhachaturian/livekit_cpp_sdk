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
#include "livekit/rtc/LiveKitRtcExport.h"
#include "livekit/rtc/stats/Stats.h"
#include <chrono>
#include <memory>
#include <vector>

namespace LiveKitCpp
{

struct StatsReportData;

class LIVEKIT_RTC_API StatsReport
{
    friend class StatsSourceImpl;
public:
    class Iterator
    {
    public:
        Iterator(const StatsReport& report, size_t index);
        Stats operator*() const { return _report.get(_index); }
        Iterator& operator++();
        Iterator& operator--();
        bool operator != (const Iterator& other) const;
    private:
        const StatsReport& _report;
        size_t _index;
    };
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
    // iterators support
    Iterator begin() const { return Iterator(*this, 0U); }
    Iterator end() const { return Iterator(*this, size()); }
private:
    StatsReport(StatsReportData* data) noexcept;
private:
    std::shared_ptr<StatsReportData> _data;
};

inline StatsReport::Iterator::Iterator(const StatsReport& report, size_t index)
    : _report(report)
    , _index(index)
{
}

inline StatsReport::Iterator& StatsReport::Iterator::operator ++ ()
{
    if (_index <= _report.size()) {
        ++_index;
    }
    return *this;
}

inline StatsReport::Iterator& StatsReport::Iterator::operator -- ()
{
    if (_index >= 0U) {
        --_index;
    }
    return *this;
}

inline bool StatsReport::Iterator::operator != (const Iterator& other) const
{
    return &_report != &other._report || _index != other._index;
}

} // namespace LiveKitCpp
