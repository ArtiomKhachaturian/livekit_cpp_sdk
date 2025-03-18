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
#include "WSAInitializer.h"

namespace LiveKitCpp
{

WSAInitializer::WSAInitializer()
{
    WSADATA wsaData;
    // request versions in descending order
    for (const auto version : {Version::v2_2, Version::v2_1, Version::v2_0,
                               Version::v1_1, Version::v1_0}) {
        _error = WsaStartup(version, wsaData);
        if (0 == _error) {
            _selectedVersion = version;
            break;
        }
    }
}

WSAInitializer::~WSAInitializer()
{
    if (0 == _error) {
        ::WSACleanup();
    }
}

std::string WSAInitializer::ToString(Version version)
{
    // The version of the Windows Sockets specification that the Ws2_32.dll expects the caller to use.
    // The high-order byte specifies the minor version number;
    // the low-order byte specifies the major version number.
    return std::to_string(LOBYTE(version)) + "." + std::to_string(HIBYTE(version));
}

int WSAInitializer::WsaStartup(Version version, WSADATA& wsaData)
{
    int error = ::WSAStartup(static_cast<WORD>(version), &wsaData);
    if (0 == error && wsaData.wVersion != static_cast<WORD>(version)) {
        error = WSAVERNOTSUPPORTED;
        ::WSACleanup();
    }
    return error;
}

} // namespace LiveKitCpp