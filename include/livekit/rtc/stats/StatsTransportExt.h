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
#pragma once // StatsTransportExt.h
#include <optional>
#include <string>

namespace LiveKitCpp
{

// https://w3c.github.io/webrtc-stats/#transportstats-dict*
class StatsTransportExt
{
public:
    virtual ~StatsTransportExt() = default;
    virtual std::optional<uint64_t> bytesSent() const = 0;
    virtual std::optional<uint64_t> packetsSent() const = 0;
    virtual std::optional<uint64_t> bytesReceived() const = 0;
    virtual std::optional<uint64_t> packetsReceived() const = 0;
    virtual std::optional<std::string> rtcpTransport_stats_id() const = 0;
    virtual std::optional<std::string> dtlsState() const = 0;
    virtual std::optional<std::string> selectedCandidatePairId() const = 0;
    virtual std::optional<std::string> localCertificateId() const = 0;
    virtual std::optional<std::string> remoteCertificateId() const = 0;
    virtual std::optional<std::string> tlsVersion() const = 0;
    virtual std::optional<std::string> dtlsCipher() const = 0;
    virtual std::optional<std::string> dtlsRole() const = 0;
    virtual std::optional<std::string> srtpCipher() const = 0;
    virtual std::optional<uint32_t> selectedCandidatePairChanges() const = 0;
    virtual std::optional<std::string> iceRole() const = 0;
    virtual std::optional<std::string> iceLocalUsernameFragment() const = 0;
    virtual std::optional<std::string> iceState() const = 0;
};

} // namespace LiveKitCpp
