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
#include "StatsIceCandidateImpl.h"
#ifdef WEBRTC_AVAILABLE

namespace LiveKitCpp
{

StatsIceCandidateImpl::StatsIceCandidateImpl(const webrtc::RTCIceCandidateStats* stats,
                                             const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCIceCandidateStats, StatsIceCandidateExt>(stats, data)
{
}

std::optional<std::string> StatsIceCandidateImpl::transportId() const
{
    if (_stats) {
        return _stats->transport_id;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::networkType() const
{
    if (_stats) {
        return _stats->network_type;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::ip() const
{
    if (_stats) {
        return _stats->ip;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::address() const
{
    if (_stats) {
        return _stats->address;
    }
    return {};
}

std::optional<int32_t> StatsIceCandidateImpl::port() const
{
    if (_stats) {
        return _stats->port;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::protocol() const
{
    if (_stats) {
        return _stats->protocol;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::relayProtocol() const
{
    if (_stats) {
        return _stats->relay_protocol;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::candidateType() const
{
    if (_stats) {
        return _stats->candidate_type;
    }
    return {};
}

std::optional<int32_t> StatsIceCandidateImpl::priority() const
{
    if (_stats) {
        return _stats->priority;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::url() const
{
    if (_stats) {
        return _stats->url;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::foundation() const
{
    if (_stats) {
        return _stats->foundation;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::relatedAddress() const
{
    if (_stats) {
        return _stats->related_address;
    }
    return {};
}

std::optional<int32_t> StatsIceCandidateImpl::relatedPort() const
{
    if (_stats) {
        return _stats->related_port;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::usernameFragment() const
{
    if (_stats) {
        return _stats->username_fragment;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::tcpType() const
{
    if (_stats) {
        return _stats->tcp_type;
    }
    return {};
}

std::optional<bool> StatsIceCandidateImpl::vpn() const
{
    if (_stats) {
        return _stats->vpn;
    }
    return {};
}

std::optional<std::string> StatsIceCandidateImpl::networkAdapterType() const
{
    if (_stats) {
        return _stats->network_adapter_type;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
