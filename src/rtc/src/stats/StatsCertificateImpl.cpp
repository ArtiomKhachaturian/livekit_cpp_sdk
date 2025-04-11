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
#include "StatsCertificateImpl.h"

namespace LiveKitCpp
{

StatsCertificateImpl::StatsCertificateImpl(const webrtc::RTCCertificateStats* stats,
                                           const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCCertificateStats, StatsCertificateExt>(stats, data)
{
}

std::optional<std::string> StatsCertificateImpl::fingerprint() const
{
    if (_stats) {
        return _stats->fingerprint;
    }
    return {};
}

std::optional<std::string> StatsCertificateImpl::fingerprintAlgorithm() const
{
    if (_stats) {
        return _stats->fingerprint_algorithm;
    }
    return {};
}

std::optional<std::string> StatsCertificateImpl::base64Certificate() const
{
    if (_stats) {
        return _stats->base64_certificate;
    }
    return {};
}

std::optional<std::string> StatsCertificateImpl::issuerCertificateId() const
{
    if (_stats) {
        return _stats->issuer_certificate_id;
    }
    return {};
}

} // namespace LiveKitCpp
