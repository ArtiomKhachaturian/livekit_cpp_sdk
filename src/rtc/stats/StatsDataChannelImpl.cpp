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
#include "StatsDataChannelImpl.h"
#ifdef WEBRTC_AVAILABLE

namespace LiveKitCpp
{

StatsDataChannelImpl::StatsDataChannelImpl(const webrtc::RTCDataChannelStats* stats,
                                           const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCDataChannelStats, StatsDataChannelExt>(stats, data)
{
}

std::optional<std::string> StatsDataChannelImpl::label() const
{
    if (_stats) {
        return _stats->label;
    }
    return {};
}

std::optional<std::string> StatsDataChannelImpl::protocol() const
{
    if (_stats) {
        return _stats->protocol;
    }
    return {};
}

std::optional<int32_t> StatsDataChannelImpl::dataChannelIdentifier() const
{
    if (_stats) {
        return _stats->data_channel_identifier;
    }
    return {};
}

std::optional<std::string> StatsDataChannelImpl::state() const
{
    if (_stats) {
        return _stats->state;
    }
    return {};
}

std::optional<uint32_t> StatsDataChannelImpl::messagesSent() const
{
    if (_stats) {
        return _stats->messages_sent;
    }
    return {};
}

std::optional<uint64_t> StatsDataChannelImpl::bytesSent() const
{
    if (_stats) {
        return _stats->bytes_sent;
    }
    return {};
}

std::optional<uint32_t> StatsDataChannelImpl::messagesReceived() const
{
    if (_stats) {
        return _stats->messages_received;
    }
    return {};
}

std::optional<uint64_t> StatsDataChannelImpl::bytesReceived() const
{
    if (_stats) {
        return _stats->bytes_received;
    }
    return {};
}

} // namespace LiveKitCpp
#endif
