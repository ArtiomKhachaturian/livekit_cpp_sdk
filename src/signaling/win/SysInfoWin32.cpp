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
#include "SysInfo.h"
#include "ScopedComInitializer.h"
#include "Utils.h"
#include "livekit/signaling/NetworkType.h"
#include <atlbase.h>
#include <Windows.h>
#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

namespace
{

inline std::string queryStr(IWbemClassObject* clsObj, LPCWSTR keyName) {
    if (clsObj && keyName) {
        VARIANT vtProp = {};
        if (SUCCEEDED(clsObj->Get(keyName, 0, &vtProp, 0, 0))) {
            auto result = LiveKitCpp::fromWideChar(std::wstring_view(vtProp.bstrVal, 
                                                                     ::SysStringLen(vtProp.bstrVal)));
            VariantClear(&vtProp);
            return result;
        }
    }
    return {};
}

}

namespace LiveKitCpp
{

// defined in NetworkType.h
NetworkType activeNetworkType()
{
    // TODO: implement it
    return NetworkType::Unknown;
}

std::string modelIdentifier()
{
    // Initialize COM
    const ScopedComInitializer com(false);
    if (!com) {
        return {};
    }
    // Initialize security
    HRESULT hres = ::CoInitializeSecurity(NULL, -1, NULL, NULL, 
                                          RPC_C_AUTHN_LEVEL_DEFAULT,
                                          RPC_C_IMP_LEVEL_IMPERSONATE, 
                                          NULL, EOAC_NONE, NULL);
    if (FAILED(hres) && RPC_E_TOO_LATE != hres) {
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
                               RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, 
                               NULL, EOAC_NONE);
    if (FAILED(hres)) {
        return {};
    }

    // Query for computer model
    CComPtr<IEnumWbemClassObject> enumerator;
    hres = svc->ExecQuery(BSTR(L"WQL"), BSTR(L"SELECT * FROM Win32_ComputerSystem"),
                          WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                          NULL, &enumerator);
    if (FAILED(hres)) {
        return {};
    }

    std::vector<std::string> modelData;
    if (enumerator) {
        IWbemClassObject* clsObj = NULL;
        ULONG uReturn = 0;
        while (enumerator->Next(WBEM_INFINITE, 1, &clsObj, &uReturn) == S_OK) {
            modelData.push_back(queryStr(clsObj, L"Manufacturer"));
            modelData.push_back(queryStr(clsObj, L"Model"));
            clsObj->Release();
            break;
        }
    }
    return join(modelData, " ", true);
}

} // namespace LiveKitCpp