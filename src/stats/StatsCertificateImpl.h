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
#pragma once // StatsCertificateImpl.h
#ifdef WEBRTC_AVAILABLE
#include "stats/StatsCertificateExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsCertificateImpl : public StatsDataImpl<webrtc::RTCCertificateStats, StatsCertificateExt>
{
public:
    StatsCertificateImpl(const webrtc::RTCCertificateStats* stats,
                         const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::Certificate; }
    // impl. of StatsCertificateExt
    std::optional<std::string> fingerprint() const final;
    std::optional<std::string> fingerprintAlgorithm() const final;
    std::optional<std::string> base64Certificate() const final;
    std::optional<std::string> issuerCertificateId() const final;
};

} // namespace LiveKitCpp
#endif
