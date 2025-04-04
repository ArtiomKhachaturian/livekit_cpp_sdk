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
#pragma once // Stats.h
#include "LiveKitClientExport.h"
#include "stats/StatsAttribute.h"
#include "stats/StatsCertificateExt.h"
#include "stats/StatsCodecExt.h"
#include "stats/StatsDataChannelExt.h"
#include "stats/StatsIceCandidateExt.h"
#include "stats/StatsIceCandidatePairExt.h"
#include "stats/StatsInboundRtpStreamExt.h"
#include "stats/StatsOutboundRtpStreamExt.h"
#include "stats/StatsPeerConnectionExt.h"
#include "stats/StatsRemoteInboundRtpStreamExt.h"
#include "stats/StatsRemoteOutboundRtpStreamExt.h"
#include "stats/StatsTransportExt.h"
#include "stats/StatsType.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace LiveKitCpp
{

class StatsData;

class LIVEKIT_CLIENT_API Stats
{
    friend class StatsReport;
public:
    Stats() = default;
    Stats(const Stats& src) noexcept;
    Stats(Stats&& tmp) noexcept;
    ~Stats();
    Stats& operator = (const Stats& src) noexcept;
    Stats& operator = (Stats&& tmp) noexcept;
    bool valid() const noexcept;
    explicit operator bool() const noexcept { return valid(); }
    const std::string& id() const;
    // Unix time in milliseconds
    std::chrono::time_point<std::chrono::system_clock> timestamp() const;
    StatsType type() const;
    std::string_view name() const;
    // Creates a JSON readable string representation of the stats
    // object, listing all of its attributes (names and values).
    std::string json() const;
    // Returns all attributes of this stats object, i.e. a list of its individual
    // metrics as viewed via the Attribute wrapper.
    std::vector<StatsAttribute> attributes() const;
    // specific data, see also StatsType description
    // StatsType::Codec
    std::shared_ptr<const StatsCodecExt> extCodec() const;
    // StatsType::Certificate
    std::shared_ptr<const StatsCertificateExt> extCertificate() const;
    // StatsType::DataChannel
    std::shared_ptr<const StatsDataChannelExt> extDataChannel() const;
    // StatsType::LocalCandidate & StatsType::RemoteCandidate
    std::shared_ptr<const StatsIceCandidateExt> extIceCandidate() const;
    // StatsType::CandidatePair
    std::shared_ptr<const StatsIceCandidatePairExt> extIceCandidatePair() const;
    // StatsType::PeerConnection
    std::shared_ptr<const StatsPeerConnectionExt> extPeerConnection() const;
    // StatsType::InboundRtp
    std::shared_ptr<const StatsInboundRtpStreamExt> extInboundRtpStream() const;
    // StatsType::OtboundRtp
    std::shared_ptr<const StatsOutboundRtpStreamExt> extOutboundRtpStream() const;
    // StatsType::RemoteInboundRtp
    std::shared_ptr<const StatsRemoteInboundRtpStreamExt> extRemoteInboundRtpStream() const;
    // StatsType::RemoteOutboundRtp
    std::shared_ptr<const StatsRemoteOutboundRtpStreamExt> extRemoteOutboundRtpStream() const;
    // StatsType::Transport
    std::shared_ptr<const StatsTransportExt> extTransport() const;
private:
    Stats(const StatsData* stats);
private:
    std::shared_ptr<const StatsData> _stats;
};

} // namespace LiveKitCpp
