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
#include "CompletionStatus.h"
#ifdef WEBRTC_WIN
#include "ComErrorHandling.h"
#else
#ifdef WEBRTC_MAC
#include <Carbon/Carbon.h>
#endif
#include "RtcUtils.h"
#endif

namespace
{

inline std::string_view formatFunctionName(bool ok, const char* function)
{
    if (!ok && function) {
        std::string_view name(function);
        auto pos = name.find('(');
        if (pos != std::string_view::npos) {
            std::string_view truncated(name.data(), pos);
            pos = truncated.rfind(' ');
            if (pos != std::string_view::npos) {
                return std::string_view(truncated.data() + pos, truncated.size() - pos);
            }
            return truncated;
        }
        return name;
    }
    return {};
}

} // namespace

namespace LiveKitCpp
{

#ifdef WEBRTC_WIN
const CompletionStatus::Code CompletionStatus::_ok = S_OK;
#elif defined(WEBRTC_MAC)
const CompletionStatus::Code CompletionStatus::_ok = noErr;
#else
const CompletionStatus::Code CompletionStatus::_ok = 0;
#endif

CompletionStatus::CompletionStatus(Code code, const char* function, int line)
    : _code(code)
    , _function(formatFunctionName(codeIsOK(_code), function))
    , _line(codeIsOK(_code) ? _noErrorLine : line)
{
}

std::string CompletionStatus::what() const
{
#ifdef WEBRTC_WIN
    return comErrorToString(code());
#else
    return osStatusToString(code());
#endif
}

CompletionStatus::Code CompletionStatus::invalidState()
{
#ifdef WEBRTC_WIN
    return E_NOT_VALID_STATE;
#elif defined(WEBRTC_MAC)
    return badReqErr;
#else
    static_assert(false, "not yet implemented");
#endif
}

CompletionStatus::Code CompletionStatus::invalidArg()
{
#ifdef WEBRTC_WIN
    return E_INVALIDARG;
#elif defined(WEBRTC_MAC)
    return paramErr;
#else
    static_assert(false, "not yet implemented");
#endif
}

bool CompletionStatus::codeIsOK(Code code)
{
#ifdef WEBRTC_WIN
    return SUCCEEDED(code);
#else
    return code == _ok;
#endif
}

} // namespace LiveKitCpp

bool operator == (const LiveKitCpp::CompletionStatus& l, const LiveKitCpp::CompletionStatus& r)
{
    return l.code() == r.code() && l.line() == r.line() && l.function() == r.function();
}

bool operator != (const LiveKitCpp::CompletionStatus& l, const LiveKitCpp::CompletionStatus& r)
{
    return l.code() != r.code() || l.line() != r.line() || l.function() != r.function();
}

std::ostream& operator << (std::ostream& os, const LiveKitCpp::CompletionStatus& status)
{
    if (!status) {
        os << "operation was not completed, error code #" << status.code();
        const auto what = status.what();
        if (!what.empty()) {
            os << ": " << what;
        }
    }
    return os;
}
