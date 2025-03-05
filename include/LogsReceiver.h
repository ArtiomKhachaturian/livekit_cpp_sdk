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
#pragma once // LogsReceiver.h
#include "LoggingSeverity.h"
#include <string_view>

namespace LiveKitCpp
{

// thread-safe implementation required
class LogsReceiver
{
public:
    virtual ~LogsReceiver() = default;
    virtual bool canLog(LoggingSeverity /*severity*/) const { return true; }
    bool canLogVerbose() const { return canLog(LoggingSeverity::Verbose); }
    bool canLogInfo() const { return canLog(LoggingSeverity::Info); }
    bool canLogWarning() const { return canLog(LoggingSeverity::Warning); }
    bool canLogError() const { return canLog(LoggingSeverity::Error); }
    virtual void onLog(LoggingSeverity severity, std::string_view log,
                       std::string_view category = {}) = 0;
    void onVerbose(std::string_view log, std::string_view category = {}) {
        onLog(LoggingSeverity::Verbose, log, category);
    }
    void onInfo(std::string_view log, std::string_view category = {}) {
        onLog(LoggingSeverity::Info, log, category);
    }
    void onWarning(std::string_view log, std::string_view category = {}) {
        onLog(LoggingSeverity::Warning, log, category);
    }
    void onError(std::string_view log, std::string_view category = {}) {
        onLog(LoggingSeverity::Error, log, category);
    }
};

} // namespace LiveKitCpp
