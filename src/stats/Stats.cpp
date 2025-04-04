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
#include "stats/Stats.h"
#include "StatsData.h"
#include "StatsReportData.h"
#ifdef WEBRTC_AVAILABLE
#include <api/stats/rtc_stats.h>
#endif

namespace
{

using namespace LiveKitCpp;

#ifdef WEBRTC_AVAILABLE
const webrtc::RTCStats* getRtcStats(const std::shared_ptr<const StatsData>& stats);
#endif

const std::string g_empty;

}

namespace LiveKitCpp
{

Stats::Stats(const StatsData* stats)
    : _stats(stats)
{
}

Stats::Stats(const Stats& src) noexcept
    : _stats(src._stats)
{
}

Stats::Stats(Stats&& tmp) noexcept
    : _stats(std::move(tmp._stats))
{
    tmp._stats.reset();
}

Stats::~Stats()
{
}

Stats& Stats::operator = (const Stats& src) noexcept
{
    if (&src != this) {
        _stats = src._stats;
    }
    return *this;
}

Stats& Stats::operator = (Stats&& tmp) noexcept
{
    if (&tmp != this) {
        _stats = std::move(tmp._stats);
        tmp._stats.reset();
    }
    return *this;
}

bool Stats::valid() const noexcept
{
#ifdef WEBRTC_AVAILABLE
    return nullptr != getRtcStats(_stats);
#endif
    return false;
}

const std::string& Stats::id() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = getRtcStats(_stats)) {
        return stats->id();
    }
#endif
    return g_empty;
}

std::chrono::time_point<std::chrono::system_clock> Stats::timestamp() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = getRtcStats(_stats)) {
        return StatsReportData::map(stats->timestamp());
    }
#endif
    return {};
}

StatsType Stats::type() const
{
#ifdef WEBRTC_AVAILABLE
    if (_stats) {
        return _stats->type();
    }
#endif
    return StatsType::Uknown;
}

std::string_view Stats::name() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = getRtcStats(_stats)) {
        return stats->type();
    }
#endif
    return {};
}

std::string Stats::json() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = getRtcStats(_stats)) {
        return stats->ToJson();
    }
#endif
    return {};
}

std::vector<StatsAttribute> Stats::attributes() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = getRtcStats(_stats)) {
        auto rtcAttributes = stats->Attributes();
        if (const auto s = rtcAttributes.size()) {
            std::vector<StatsAttribute> attributes;
            attributes.reserve(s);
            for (size_t i = 0U; i < s; ++i) {
                const auto& attribute = rtcAttributes.at(i);
                attributes.push_back(StatsAttribute{attribute.name(), std::move(attribute.as_variant())});
            }
            return attributes;
        }
    }
#endif
    return {};
}

std::shared_ptr<const StatsCodecExt> Stats::extCodec() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsCodecExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsCertificateExt> Stats::extCertificate() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsCertificateExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsDataChannelExt> Stats::extDataChannel() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsDataChannelExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsIceCandidateExt> Stats::extIceCandidate() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsIceCandidateExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsIceCandidatePairExt> Stats::extIceCandidatePair() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsIceCandidatePairExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsPeerConnectionExt> Stats::extPeerConnection() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsPeerConnectionExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsInboundRtpStreamExt> Stats::extInboundRtpStream() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsInboundRtpStreamExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsOutboundRtpStreamExt> Stats::extOutboundRtpStream() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsOutboundRtpStreamExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsRemoteInboundRtpStreamExt> Stats::extRemoteInboundRtpStream() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsRemoteInboundRtpStreamExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsRemoteOutboundRtpStreamExt> Stats::extRemoteOutboundRtpStream() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsRemoteOutboundRtpStreamExt>(_stats);
#endif
    return {};
}

std::shared_ptr<const StatsTransportExt> Stats::extTransport() const
{
#ifdef WEBRTC_AVAILABLE
    return std::dynamic_pointer_cast<const StatsTransportExt>(_stats);
#endif
    return {};
}

} // namespace LiveKitCpp

#ifdef WEBRTC_AVAILABLE
namespace
{

const webrtc::RTCStats* getRtcStats(const std::shared_ptr<const StatsData>& stats)
{
    if (stats) {
        return stats->rtcStats();
    }
    return nullptr;
}

}
#endif
