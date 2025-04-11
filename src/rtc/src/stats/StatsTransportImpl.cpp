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
#include "StatsTransportImpl.h"

namespace LiveKitCpp
{

StatsTransportImpl::StatsTransportImpl(const webrtc::RTCTransportStats* stats,
                                       const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCTransportStats, StatsTransportExt>(stats, data)
{
}

std::optional<uint64_t> StatsTransportImpl::bytesSent() const
{
    if (_stats) {
        return _stats->bytes_sent;
    }
    return {};
}

std::optional<uint64_t> StatsTransportImpl::packetsSent() const
{
    if (_stats) {
        return _stats->packets_sent;
    }
    return {};
}

std::optional<uint64_t> StatsTransportImpl::bytesReceived() const
{
    if (_stats) {
        return _stats->bytes_received;
    }
    return {};
}

std::optional<uint64_t> StatsTransportImpl::packetsReceived() const
{
    if (_stats) {
        return _stats->packets_received;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::rtcpTransport_stats_id() const
{
    if (_stats) {
        return _stats->rtcp_transport_stats_id;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::dtlsState() const
{
    if (_stats) {
        return _stats->dtls_state;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::selectedCandidatePairId() const
{
    if (_stats) {
        return _stats->selected_candidate_pair_id;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::localCertificateId() const
{
    if (_stats) {
        return _stats->local_certificate_id;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::remoteCertificateId() const
{
    if (_stats) {
        return _stats->remote_certificate_id;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::tlsVersion() const
{
    if (_stats) {
        return _stats->tls_version;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::dtlsCipher() const
{
    if (_stats) {
        return _stats->dtls_cipher;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::dtlsRole() const
{
    if (_stats) {
        return _stats->dtls_role;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::srtpCipher() const
{
    if (_stats) {
        return _stats->srtp_cipher;
    }
    return {};
}

std::optional<uint32_t> StatsTransportImpl::selectedCandidatePairChanges() const
{
    if (_stats) {
        return _stats->selected_candidate_pair_changes;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::iceRole() const
{
    if (_stats) {
        return _stats->ice_role;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::iceLocalUsernameFragment() const
{
    if (_stats) {
        return _stats->ice_local_username_fragment;
    }
    return {};
}

std::optional<std::string> StatsTransportImpl::iceState() const
{
    if (_stats) {
        return _stats->ice_state;
    }
    return {};
}

} // namespace LiveKitCpp
