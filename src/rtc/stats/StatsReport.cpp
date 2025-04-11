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
#include "stats/StatsReport.h"
#include "StatsReportData.h"
#include "StatsData.h"
#ifdef WEBRTC_AVAILABLE
#include "StatsAudioPlayoutImpl.h"
#include "StatsAudioSourceImpl.h"
#include "StatsCertificateImpl.h"
#include "StatsCodecImpl.h"
#include "StatsDataChannelImpl.h"
#include "StatsIceCandidatePairImpl.h"
#include "StatsIceCandidateImpl.h"
#include "StatsInboundRtpStreamImpl.h"
#include "StatsOutboundRtpStreamImpl.h"
#include "StatsPeerConnectionImpl.h"
#include "StatsRemoteInboundRtpStreamImpl.h"
#include "StatsRemoteOutboundRtpStreamImpl.h"
#include "StatsTransportImpl.h"
#include "StatsVideoSourceImpl.h"
#endif
#include "Utils.h"
#include <cassert>

#ifdef WEBRTC_AVAILABLE
namespace
{

using namespace LiveKitCpp;

StatsData* make(const webrtc::RTCStats* stats,
                const std::shared_ptr<const StatsReportData>& data);

}
#endif

namespace LiveKitCpp
{

StatsReport::StatsReport(StatsReportData* data) noexcept
    : _data(data)
{
}

StatsReport::StatsReport(const StatsReport& src) noexcept
    : _data(src._data)
{
}

StatsReport::StatsReport(StatsReport&& tmp) noexcept
    : _data(std::move(tmp._data))
{
}

StatsReport::~StatsReport()
{
}

StatsReport& StatsReport::operator = (const StatsReport& src) noexcept
{
    if (&src != this) {
        _data = src._data;
    }
    return *this;
}

StatsReport& StatsReport::operator = (StatsReport&& tmp) noexcept
{
    if (&tmp != this) {
        _data = std::move(tmp._data);
    }
    return *this;
}

std::chrono::time_point<std::chrono::system_clock> StatsReport::timestamp() const
{
#ifdef WEBRTC_AVAILABLE
    if (_data && _data->_data) {
        return StatsReportData::map(_data->_data->timestamp());
    }
#endif
    return {};
}

size_t StatsReport::size() const
{
#ifdef WEBRTC_AVAILABLE
    if (_data && _data->_data) {
        return _data->_data->size();
    }
#endif
    return 0U;
}

Stats StatsReport::get(const std::string& id) const
{
#ifdef WEBRTC_AVAILABLE
    if (_data && _data->_data) {
        if (const auto stats = _data->_data->Get(id)) {
            return Stats(make(stats, _data));
        }
    }
#endif
    return {};
}

Stats StatsReport::get(size_t index) const
{
#ifdef WEBRTC_AVAILABLE
    if (_data && _data->_data && index < _data->_data->size()) {
        size_t i = 0U;
        for (auto it = _data->_data->begin(); it != _data->_data->end(); ++it, ++i) {
            if (i == index) {
                return Stats(make(it.operator->(), _data));
            }
        }
    }
#endif
    return {};
}

std::string StatsReport::json() const
{
#ifdef WEBRTC_AVAILABLE
    if (_data && _data->_data) {
        return _data->_data->ToJson();
    }
#endif
    return {};
}

#ifdef WEBRTC_AVAILABLE
std::chrono::time_point<std::chrono::system_clock> StatsReportData::map(const webrtc::Timestamp& t)
{
    using namespace std::chrono;
    const auto us = duration_cast<system_clock::duration>(nanoseconds(t.us()));
    return time_point<system_clock>(us);
}
#endif

std::string toString(StatsType type)
{
    switch (type) {
        case StatsType::Uknown:
            break;
        case StatsType::Codec:
            return "codec";
        case StatsType::InboundRtp:
            return "inbound-rtp";
        case StatsType::OutboundRtp:
            return "outbound-rtp";
        case StatsType::RemoteInboundRtp:
            return "remote-inbound-rtp";
        case StatsType::RemoteOutboundRtp:
            return "remote-outbound-rtp";
        case StatsType::MediaSource:
            return "media-source";
        case StatsType::MediaPlayout:
            return "media-playout";
        case StatsType::PeerConnection:
            return "peer-connection";
        case StatsType::DataChannel:
            return "data-channel";
        case StatsType::Transport:
            return "transport";
        case StatsType::CandidatePair:
            return "candidate-pair";
        case StatsType::LocalCandidate:
            return "local-candidate";
        case StatsType::RemoteCandidate:
            return "remote-candidate";
        case StatsType::Certificate:
            return "certificate";
        default:
            assert(false);
            break;
    }
    return {};
}

StatsType StatsData::type() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = rtcStats()) {
        return toStatsType(stats->id());
    }
#endif
    return StatsType::Uknown;
}

StatsType toStatsType(std::string_view type)
{
    if (!type.empty()) {
        static const StatsType statTypes [] = {
            StatsType::Codec,
            StatsType::InboundRtp,
            StatsType::OutboundRtp,
            StatsType::RemoteInboundRtp,
            StatsType::RemoteOutboundRtp,
            StatsType::MediaSource,
            StatsType::MediaPlayout,
            StatsType::PeerConnection,
            StatsType::DataChannel,
            StatsType::Transport,
            StatsType::CandidatePair,
            StatsType::LocalCandidate,
            StatsType::RemoteCandidate,
            StatsType::Certificate
        };
        for (auto statType : statTypes) {
            if (compareCaseSensitive(type, toString(statType))) {
                return statType;
            }
        }
    }
    return StatsType::Uknown;
}

} // namespace LiveKitCpp

#ifdef WEBRTC_AVAILABLE
namespace
{

StatsData* make(const webrtc::RTCStats* stats,
                const std::shared_ptr<const StatsReportData>& data)
{
    if (stats) {
        switch (toStatsType(stats->type())) {
            case LiveKitCpp::StatsType::Codec:
                if (const auto cs = dynamic_cast<const webrtc::RTCCodecStats*>(stats)) {
                    return new StatsCodecImpl(cs, data);
                }
                break;
            case LiveKitCpp::StatsType::InboundRtp:
                if (const auto is = dynamic_cast<const webrtc::RTCInboundRtpStreamStats*>(stats)) {
                    return new StatsInboundRtpStreamImpl(is, data);
                }
                break;
            case LiveKitCpp::StatsType::OutboundRtp:
                if (const auto os = dynamic_cast<const webrtc::RTCOutboundRtpStreamStats*>(stats)) {
                    return new StatsOutboundRtpStreamImpl(os, data);
                }
                break;
            case LiveKitCpp::StatsType::RemoteInboundRtp:
                if (const auto ris = dynamic_cast<const webrtc::RTCRemoteInboundRtpStreamStats*>(stats)) {
                    return new StatsRemoteInboundRtpStreamImpl(ris, data);
                }
                break;
            case LiveKitCpp::StatsType::RemoteOutboundRtp:
                if (const auto ros = dynamic_cast<const webrtc::RTCRemoteOutboundRtpStreamStats*>(stats)) {
                    return new StatsRemoteOutboundRtpStreamImpl(ros, data);
                }
                break;
            case LiveKitCpp::StatsType::MediaSource:
                if (const auto as = dynamic_cast<const webrtc::RTCAudioSourceStats*>(stats)) {
                    return new StatsAudioSourceImpl(as, data);
                }
                if (const auto vs = dynamic_cast<const webrtc::RTCVideoSourceStats*>(stats)) {
                    return new StatsVideoSourceImpl(vs, data);
                }
                if (const auto ms = dynamic_cast<const webrtc::RTCMediaSourceStats*>(stats)) {
                    return new StatsMediaSourceImpl(ms, data);
                }
                break;
            case LiveKitCpp::StatsType::MediaPlayout:
                if (const auto aps = dynamic_cast<const webrtc::RTCAudioPlayoutStats*>(stats)) {
                    return new StatsAudioPlayoutImpl(aps, data);
                }
                break;
            case LiveKitCpp::StatsType::PeerConnection:
                if (const auto pcs = dynamic_cast<const webrtc::RTCPeerConnectionStats*>(stats)) {
                    return new StatsPeerConnectionImpl(pcs, data);
                }
                break;
            case LiveKitCpp::StatsType::DataChannel:
                if (const auto ds = dynamic_cast<const webrtc::RTCDataChannelStats*>(stats)) {
                    return new StatsDataChannelImpl(ds, data);
                }
                break;
            case LiveKitCpp::StatsType::Transport:
                if (const auto ts = dynamic_cast<const webrtc::RTCTransportStats*>(stats)) {
                    return new StatsTransportImpl(ts, data);
                }
                break;
            case LiveKitCpp::StatsType::CandidatePair:
                if (const auto icps = dynamic_cast<const webrtc::RTCIceCandidatePairStats*>(stats)) {
                    return new StatsIceCandidatePairImpl(icps, data);
                }
                break;
            case LiveKitCpp::StatsType::LocalCandidate:
            case LiveKitCpp::StatsType::RemoteCandidate:
                if (const auto ics = dynamic_cast<const webrtc::RTCIceCandidateStats*>(stats)) {
                    return new StatsIceCandidateImpl(ics, data);
                }
                break;
            case LiveKitCpp::StatsType::Certificate:
                if (const auto cs = dynamic_cast<const webrtc::RTCCertificateStats*>(stats)) {
                    return new StatsCertificateImpl(cs, data);
                }
                break;
            default:
                break;
        }
        return new StatsDataImpl<webrtc::RTCStats>(stats, data);
    }
    return nullptr;
}

}
#endif
