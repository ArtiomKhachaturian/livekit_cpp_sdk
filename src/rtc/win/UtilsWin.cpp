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
#include "Utils.h"
#include "ComErrorHandling.h"
#include "Logger.h"
#ifdef WEBRTC_WIN
#include <rtc_base/string_utils.h>
#include <rtc_base/strings/string_builder.h>
#endif
#include <atlbase.h>
#include <Windows.h>
#include <wbemidl.h>

#ifdef WIN32
typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
#pragma comment(lib, "wbemuuid.lib")
#endif

namespace LiveKitCpp
{

std::vector<BYTE> queryRegistryValue(HKEY root, LPCSTR lpSubKey, LPCSTR lpValueName, LPDWORD lpType)
{
    std::vector<BYTE> data;
    if (root) {
        HKEY hkey = NULL;
        auto status = ::RegOpenKeyExA(root, lpSubKey, 0, KEY_QUERY_VALUE, &hkey);
        if (ERROR_SUCCESS == status) {
            DWORD dataSize = 0;
            status = ::RegQueryValueExA(hkey, lpValueName, NULL, lpType, NULL, &dataSize);
            if (ERROR_SUCCESS == status) {
                data.resize(dataSize);
                status = ::RegQueryValueExA(hkey, lpValueName, NULL, NULL, data.data(), &dataSize);
                if (ERROR_SUCCESS != status) {
                    // log warn
                    data.clear();
                }
            }
        }
        if (hkey) {
            ::RegCloseKey(hkey);
        }
    }
    return data;
}

#ifdef WEBRTC_WIN
std::string comErrorToString(const _com_error& error, const char* function, int lineNumber)
{
    thread_local static char ss_buf[1024];
    rtc::SimpleStringBuilder ss(ss_buf);
    ss.AppendFormat("%s (0x%08X)", rtc::ToUtf8(error.ErrorMessage()).c_str(), error.Error());
    if (function) {
        const std::string_view functionName(function);
        if (!functionName.empty()) {
            ss << "[" << functionName;
            if (lineNumber > 0) {
                ss << ", line #" << lineNumber;
            }
            ss << "]";
        }
    }
    return ss.str();
}

std::string comErrorToString(HRESULT hr, const char* function, int lineNumber)
{
    if (FAILED(hr)) {
        return comErrorToString(_com_error(hr), function, lineNumber);
    }
    return {};
}

std::string comErrorToString(const std::system_error& e, const char* function, int lineNumber)
{
    const HRESULT code(e.code().value());
    if (FAILED(code)) {
        std::string error(e.what());
        if (!error.empty()) {
            const auto desc = comErrorToString(code, function, lineNumber);
            if (!desc.empty()) {
                error += " - " + desc;
            }
        }
        return error;
    }
    return {};
}

HRESULT logComError(HRESULT hr, const char* function, int lineNumber,
                    const std::shared_ptr<Bricks::Logger>& logger,
                    std::string_view category)
{
    if (FAILED(hr) && logger && logger->canLogError()) {
        const auto error = comErrorToString(hr, function, lineNumber);
        logger->logError(error, category);
    }
    return hr;
}
#endif

} // namespace LiveKitCpp