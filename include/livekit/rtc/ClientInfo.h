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
#pragma once // ClientInfo.h
#include "livekit/LiveKitClientExport.h"
#include "livekit/rtc/Sdk.h"
#include <string>

namespace LiveKitCpp
{

// details about the client
struct LIVEKIT_CLIENT_API ClientInfo
{
    ClientInfo();
    SDK _sdk = SDK::CPP;
    std::string _version;
    /// LiveKit server protocol version to use. Generally, it's not recommended to change this.
    int32_t _protocol;
    std::string _os;
    std::string _osVersion;
    std::string _deviceModel;
    std::string _browser;
    std::string _browserVersion;
    std::string _address;
    // wifi, wired, cellular, vpn, empty if not known
    std::string _network;
    // comma separated list of additional LiveKit SDKs in use of this client, with versions
    // e.g. "components-js:1.2.3,track-processors-js:1.2.3"
    std::string _otherSdks;
    static ClientInfo defaultClientInfo();
};

} // namespace LiveKitCpp
