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
#pragma once // WebRtcLogSink.h
#include <rtc_base/logging.h>
#include <optional>

namespace LiveKitCpp
{

class WebRtcLogSink : public rtc::LogSink
{
public:
    WebRtcLogSink();
    ~WebRtcLogSink() override;
private:
    // impl. of rtc::LogSink
    void OnLogMessage(const std::string& message,
                      rtc::LoggingSeverity severity)  final;
    void OnLogMessage(const std::string& message) final;
    void OnLogMessage(absl::string_view message,
                      rtc::LoggingSeverity severity) final;
    void OnLogMessage(absl::string_view message) final;
};

} // namespace LiveKitCpp
