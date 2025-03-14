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
#include "TransportState.h"
#include "NetworkType.h"
#ifdef _WIN32
#include <atlbase.h>
#include <Windows.h>
#include <wbemidl.h>
#else
#include <pthread.h>
#ifndef __APPLE__
#include <sys/prctl.h>
#endif
#endif
#include <codecvt>
#include <locale>

#ifdef WIN32
typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
#pragma comment(lib, "wbemuuid.lib")
#endif

namespace {

#ifdef WIN32
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
#endif

}

using ConvertType = std::codecvt_utf8<wchar_t>;

namespace LiveKitCpp
{

#ifdef _WIN32
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

#endif

std::string fromWideChar(const std::wstring& w)
{
    if (!w.empty()) {
        std::wstring_convert<ConvertType, wchar_t> converter;
        return converter.to_bytes(w);
    }
    return {};
}

template<typename TPCEnum>
inline std::string stateToString(TPCEnum state) { return {}; }

template<typename TPCEnum>
inline std::string enumTypeToString() { return {}; }

template<>
inline std::string stateToString<TransportState>(TransportState state) {
    return toString(state);
}

template<>
inline std::string enumTypeToString<TransportState>() {
    return "transport";
}

template<typename TPCEnum>
inline std::string makeChangesString(TPCEnum from, TPCEnum to) {
    return enumTypeToString<TPCEnum>() + " state changed from '" +
           stateToString(from) + "' to '" + stateToString(to) + "'";
}

std::string makeStateChangesString(TransportState from, TransportState to)
{
    return makeChangesString(from, to);
}

std::string toString(NetworkType state)
{
    switch (state) {
        case NetworkType::WiFi:
            return "wifi";
        case NetworkType::Wired:
            return "wired";
        case NetworkType::Cellular:
            return "cellular";
        case NetworkType::Vpn:
            return "vpn";
        default:
            break;
    }
    return "";
}

} // namespace LiveKitCpp

#ifdef WIN32
namespace {

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
#endif
