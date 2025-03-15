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
#include "NetworkType.h"
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

namespace 
{

class ScopedComInitializer
{
public:
    ScopedComInitializer();
    ~ScopedComInitializer();
    bool ok() const noexcept { return SUCCEEDED(_hr); }
    explicit operator bool() const noexcept { return ok(); }
private:
    const HRESULT _hr;
};

}

namespace LiveKitCpp
{

std::string operatingSystemVersion()
{
    if (HMODULE hNtdll = ::GetModuleHandleA("ntdll.dll")) {
        if (auto RtlGetVersion = (RtlGetVersionPtr)::GetProcAddress(hNtdll,
            "RtlGetVersion")) {
            RTL_OSVERSIONINFOEXW osInfo = { 0 };
            osInfo.dwOSVersionInfoSize = sizeof(osInfo);
            if (0 == RtlGetVersion(&osInfo)) {
                return std::to_string(osInfo.dwMajorVersion) + "." +
                    std::to_string(osInfo.dwMinorVersion);
            }
        }
    }
    return {};
}

std::string operatingSystemName()
{
    return "Windows";
}

std::string modelIdentifier()
{
    // Initialize COM
    const ScopedComInitializer com;
    if (!com) {
        return {};
    }
    // Initialize security
    HRESULT hres = ::CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) {
        return {};
    }

    // Connect to WMI
    CComPtr<IWbemLocator> loc;
    hres = ::CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&loc);
    if (FAILED(hres)) {
        return {};
    }

    CComPtr<IWbemServices> svc;
    hres = loc->ConnectServer(BSTR(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &svc);
    if (FAILED(hres)) {
        return {};
    }

    // Set security levels
    hres = ::CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        return {};
    }

    // Query for computer model
    CComPtr<IEnumWbemClassObject> enumerator;
    hres = svc->ExecQuery(BSTR(L"WQL"), BSTR(L"SELECT * FROM Win32_ComputerSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
    if (FAILED(hres)) {
        return {};
    }

    std::wstring model;
    if (enumerator) {
        IWbemClassObject* clsObj = NULL;
        ULONG uReturn = 0;
        while (enumerator->Next(WBEM_INFINITE, 1, &clsObj, &uReturn) == S_OK) {
            VARIANT vtProp = {};
            clsObj->Get(L"Model", 0, &vtProp, 0, 0);
            model = std::wstring(vtProp.bstrVal, ::SysStringLen(vtProp.bstrVal));
            VariantClear(&vtProp);
            clsObj->Release();
            break;
        }
    }
    return fromWideChar(model);
}

NetworkType activeNetworkType()
{
    // TODO: implement it
    return NetworkType::Unknown;
}

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

namespace 
{

ScopedComInitializer::ScopedComInitializer()
    : _hr(::CoInitializeEx(0, COINIT_MULTITHREADED))
{
}

ScopedComInitializer::~ScopedComInitializer()
{
    if (ok()) {
        ::CoUninitialize();
    }
}

}
