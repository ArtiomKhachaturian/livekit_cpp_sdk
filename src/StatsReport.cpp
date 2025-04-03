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
#include "Utils.h"
#include <cassert>

namespace
{

#ifdef WEBRTC_AVAILABLE
inline std::chrono::time_point<std::chrono::system_clock> map(const webrtc::Timestamp& t) {
    using namespace std::chrono;
    const auto us = duration_cast<system_clock::duration>(nanoseconds(t.us()));
    return time_point<system_clock>(us);
}
#endif

struct VisitIsSequence
{
  // Any type of vector is a sequence.
    template <typename T>
    bool operator()(const std::optional<std::vector<T>>*) const { return true; }
    // Any other type is not.
    template <typename T>
    bool operator()(const std::optional<T>*) const { return false; }
};

struct VisitIsMap
{
  // Any type of vector is a sequence.
    template <typename T>
    bool operator()(const std::optional<std::map<std::string, T>>*) const { return true; }
    // Any other type is not.
    template<typename T>
    bool operator()(const std::optional<T>*) const { return false; }
};

struct VisitToString
{
    template<typename T>
    std::string operator()(const std::optional<T>* attr) const;
private:
    static const std::string& toString(const std::string& s) { return s; }
    static std::string toString(bool v) { return v ? "true" : "false"; }
    static std::string toString(int32_t v) { return std::to_string(v); }
    static std::string toString(uint32_t v) { return std::to_string(v); }
    static std::string toString(int64_t v) { return std::to_string(v); }
    static std::string toString(uint64_t v) { return std::to_string(v); }
    static std::string toString(double v) { return std::to_string(v); }
    template<typename T>
    static std::string toString(const std::vector<T>& v);
    template<typename T>
    static std::string toString(const std::map<std::string, T>& v);
    static std::string join(const std::vector<std::string>& strings);
};

const std::string g_empty;

}

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
        return map(_data->_data->timestamp());
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
            return Stats(stats, _data);
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
                return Stats(it.operator->(), _data);
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

Stats::Stats(const void* stats, const std::shared_ptr<StatsReportData>& data)
    : _stats(stats)
    , _data(data)
{
}

Stats::Stats(const Stats& src) noexcept
    : _stats(src._stats)
    , _data(src._data)
{
}

Stats::Stats(Stats&& tmp) noexcept
    : _stats(tmp._stats)
    , _data(std::move(tmp._data))
{
    tmp._stats = nullptr;
}

Stats::~Stats()
{
}

Stats& Stats::operator = (const Stats& src) noexcept
{
    if (&src != this) {
        _stats = src._stats;
        _data = src._data;
    }
    return *this;
}

Stats& Stats::operator = (Stats&& tmp) noexcept
{
    if (&tmp != this) {
        _stats = tmp._stats;
        _data = std::move(tmp._data);
        tmp._stats = nullptr;
    }
    return *this;
}

bool Stats::valid() const noexcept
{
    return nullptr != _stats;
}

const std::string& Stats::id() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = reinterpret_cast<const webrtc::RTCStats*>(_stats)) {
        return stats->id();
    }
#endif
    return g_empty;
}

std::chrono::time_point<std::chrono::system_clock> Stats::timestamp() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = reinterpret_cast<const webrtc::RTCStats*>(_stats)) {
        return map(stats->timestamp());
    }
#endif
    return {};
}

std::string_view Stats::type() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = reinterpret_cast<const webrtc::RTCStats*>(_stats)) {
        return stats->type();
    }
#endif
    return {};
}

std::string Stats::json() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = reinterpret_cast<const webrtc::RTCStats*>(_stats)) {
        return stats->ToJson();
    }
#endif
    return {};
}

std::vector<StatsAttribute> Stats::attributes() const
{
#ifdef WEBRTC_AVAILABLE
    if (const auto stats = reinterpret_cast<const webrtc::RTCStats*>(_stats)) {
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

StatsAttribute::StatsAttribute(std::string_view name, Value value)
    : _name(std::move(name))
    , _value(std::move(value))
{
}

std::string_view StatsAttribute::name() const
{
    return _name;
}

bool StatsAttribute::valid() const
{
    return std::visit([](const auto* attr) { return attr->has_value(); }, _value);
}

bool StatsAttribute::isSequence() const
{
    return std::visit(VisitIsSequence{}, _value);
}

bool StatsAttribute::isAssociative() const
{
    return std::visit(VisitIsMap{}, _value);
}

std::string StatsAttribute::toString() const
{
    if (valid()) {
        return std::visit(VisitToString{}, _value);
    }
    return {};
}

bool StatsAttribute::operator == (const StatsAttribute& other) const
{
    if (&other != this) {
        return _value == other._value;
    }
    return true;
}

bool StatsAttribute::operator != (const StatsAttribute& other) const
{
    return _value != other._value;
}

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

StatsType fromString(std::string_view type)
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

namespace
{

template<typename T>
std::string VisitToString::operator()(const std::optional<T>* attr) const
{
    if (attr && attr->has_value()) {
        return toString(attr->value());
    }
    return {};
}

template<typename T>
std::string VisitToString::toString(const std::vector<T>& v)
{
    if (const auto s = v.size()) {
        std::vector<std::string> strings;
        strings.reserve(s);
        for (size_t i = 0U; i < s; ++i) {
            strings.push_back(toString(v.at(i)));
        }
        return join(strings);
    }
    return {};
}

template<typename T>
std::string VisitToString::toString(const std::map<std::string, T>& v)
{
    if (const auto s = v.size()) {
        std::vector<std::string> strings;
        strings.reserve(s);
        for (auto it = v.begin(); it != v.end(); ++it) {
            strings.push_back(it->first + ":" + toString(it->second));
        }
        return join(strings);
    }
    return {};
}

std::string VisitToString::join(const std::vector<std::string>& strings)
{
    return LiveKitCpp::join(strings, ",", false);
}

}
