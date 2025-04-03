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
#pragma once // StatsIceCandidateExt.h
#include <optional>
#include <string>

namespace LiveKitCpp
{

// https://www.w3.org/TR/webrtc-stats/#dom-rtcicecandidatestats
class StatsIceCandidateExt
{
public:
    virtual ~StatsIceCandidateExt() = default;
    virtual std::optional<std::string> transportId() const = 0;
    virtual std::optional<std::string> networkType() const = 0;
    virtual std::optional<std::string> ip() const = 0;
    virtual std::optional<std::string> address() const = 0;
    virtual std::optional<int32_t> port() const = 0;
    virtual std::optional<std::string> protocol() const = 0;
    virtual std::optional<std::string> relayProtocol() const = 0;
    virtual std::optional<std::string> candidateType() const = 0;
    virtual std::optional<int32_t> priority() const = 0;
    virtual std::optional<std::string> url() const = 0;
    virtual std::optional<std::string> foundation() const = 0;
    virtual std::optional<std::string> relatedAddress() const = 0;
    virtual std::optional<int32_t> relatedPort() const = 0;
    virtual std::optional<std::string> usernameFragment() const = 0;
    virtual std::optional<std::string> tcpType() const = 0;
    virtual std::optional<bool> vpn() const = 0;
    virtual std::optional<std::string> networkAdapterType() const = 0;
};

} // namespace LiveKitCpp
