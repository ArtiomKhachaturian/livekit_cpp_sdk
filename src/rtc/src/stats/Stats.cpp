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
#include "livekit/rtc/stats/Stats.h"
#include "StatsData.h"
#include "StatsReportData.h"
#include <api/stats/rtc_stats.h>

namespace
{

using namespace LiveKitCpp;

const webrtc::RTCStats* getRtcStats(const std::shared_ptr<const StatsData>& stats);

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
    return nullptr != getRtcStats(_stats);
}

const std::string& Stats::id() const
{
    if (const auto stats = getRtcStats(_stats)) {
        return stats->id();
    }
}

std::chrono::time_point<std::chrono::system_clock> Stats::timestamp() const
{
    if (const auto stats = getRtcStats(_stats)) {
        return StatsReportData::map(stats->timestamp());
    }
    return {};
}

StatsType Stats::type() const
{
    if (_stats) {
        return _stats->type();
    }
    return StatsType::Uknown;
}

std::string_view Stats::name() const
{
    if (const auto stats = getRtcStats(_stats)) {
        return stats->type();
    }
    return {};
}

std::string Stats::json() const
{
    if (const auto stats = getRtcStats(_stats)) {
        return stats->ToJson();
    }
    return {};
}

std::vector<StatsAttribute> Stats::attributes() const
{
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
    return {};
}

std::shared_ptr<const StatsCodecExt> Stats::extCodec() const
{
    return std::dynamic_pointer_cast<const StatsCodecExt>(_stats);
}

std::shared_ptr<const StatsCertificateExt> Stats::extCertificate() const
{
    return std::dynamic_pointer_cast<const StatsCertificateExt>(_stats);
}

std::shared_ptr<const StatsDataChannelExt> Stats::extDataChannel() const
{
    return std::dynamic_pointer_cast<const StatsDataChannelExt>(_stats);
}

std::shared_ptr<const StatsIceCandidateExt> Stats::extIceCandidate() const
{
    return std::dynamic_pointer_cast<const StatsIceCandidateExt>(_stats);
}

std::shared_ptr<const StatsIceCandidatePairExt> Stats::extIceCandidatePair() const
{
    return std::dynamic_pointer_cast<const StatsIceCandidatePairExt>(_stats);
}

std::shared_ptr<const StatsPeerConnectionExt> Stats::extPeerConnection() const
{
    return std::dynamic_pointer_cast<const StatsPeerConnectionExt>(_stats);
}

std::shared_ptr<const StatsInboundRtpStreamExt> Stats::extInboundRtpStream() const
{
    return std::dynamic_pointer_cast<const StatsInboundRtpStreamExt>(_stats);
}

std::shared_ptr<const StatsOutboundRtpStreamExt> Stats::extOutboundRtpStream() const
{
    return std::dynamic_pointer_cast<const StatsOutboundRtpStreamExt>(_stats);
}

std::shared_ptr<const StatsRemoteInboundRtpStreamExt> Stats::extRemoteInboundRtpStream() const
{
    return std::dynamic_pointer_cast<const StatsRemoteInboundRtpStreamExt>(_stats);
}

std::shared_ptr<const StatsRemoteOutboundRtpStreamExt> Stats::extRemoteOutboundRtpStream() const
{
    return std::dynamic_pointer_cast<const StatsRemoteOutboundRtpStreamExt>(_stats);
}

std::shared_ptr<const StatsTransportExt> Stats::extTransport() const
{
    return std::dynamic_pointer_cast<const StatsTransportExt>(_stats);
}

std::shared_ptr<const StatsAudioPlayoutExt> Stats::extAudioPlayout() const
{
    return std::dynamic_pointer_cast<const StatsAudioPlayoutExt>(_stats);
}

std::shared_ptr<const StatsMediaSourceExt> Stats::extMediaSource() const
{
    return std::dynamic_pointer_cast<const StatsMediaSourceExt>(_stats);
}

std::shared_ptr<const StatsAudioSourceExt> Stats::extAudioSource() const
{
    return std::dynamic_pointer_cast<const StatsAudioSourceExt>(_stats);
}

std::shared_ptr<const StatsVideoSourceExt> Stats::extVideoSource() const
{
    return std::dynamic_pointer_cast<const StatsVideoSourceExt>(_stats);
}

} // namespace LiveKitCpp

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
