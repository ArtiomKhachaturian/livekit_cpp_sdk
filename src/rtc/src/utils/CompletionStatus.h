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
#pragma once // CompletionStatus.h
#ifdef WEBRTC_WIN
#include <Windows.h>
#elif defined(WEBRTC_MAC)
#include <MacTypes.h>
#endif
#include <string>
#include <sstream>

namespace LiveKitCpp
{

class CompletionStatus
{
public:
#ifdef WEBRTC_WIN
    using Code = HRESULT;
#elif defined(WEBRTC_MAC)
    using Code = OSStatus;
#else
    using Code = int;
#endif
public:
    CompletionStatus() = default;
    explicit CompletionStatus(Code code, const char* function = "", int line = -1);
    CompletionStatus(const CompletionStatus&) = default;
    CompletionStatus(CompletionStatus&&) = default;
    CompletionStatus& operator=(const CompletionStatus&) = default;
    CompletionStatus& operator=(CompletionStatus&&) = default;
    std::string what() const;
    auto code() const noexcept { return _code; }
    const auto& function() const noexcept { return _function; }
    int line() const noexcept { return _line; }
    bool ok() const noexcept { return codeIsOK(code()); }
    explicit operator bool() const noexcept { return ok(); }
    static CompletionStatus::Code invalidState();
    static CompletionStatus::Code invalidArg();
private:
    constexpr static bool codeIsOK(Code code);
private:
    static constexpr const int _noErrorLine = -1;
    static const Code _ok;
    Code _code = _ok;
    std::string_view _function;
    int _line = _noErrorLine;
};

} // namespace LiveKitCpp

bool operator == (const LiveKitCpp::CompletionStatus& l, const LiveKitCpp::CompletionStatus& r);
bool operator != (const LiveKitCpp::CompletionStatus& l, const LiveKitCpp::CompletionStatus& r);
std::ostream& operator << (std::ostream& os, const LiveKitCpp::CompletionStatus& status);

#define COMPLETION_STATUS(status) LiveKitCpp::CompletionStatus(status, __PRETTY_FUNCTION__, __LINE__)
#define COMPLETION_STATUS_INVALID_STATE COMPLETION_STATUS(LiveKitCpp::CompletionStatus::invalidState())
#define COMPLETION_STATUS_INVALID_ARG COMPLETION_STATUS(LiveKitCpp::CompletionStatus::invalidArg())
