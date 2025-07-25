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
#include "StatsPeerConnectionImpl.h"

namespace LiveKitCpp
{

StatsPeerConnectionImpl::StatsPeerConnectionImpl(const webrtc::RTCPeerConnectionStats* stats,
                                                 const std::shared_ptr<const StatsReportData>& data)
    : StatsDataImpl<webrtc::RTCPeerConnectionStats, StatsPeerConnectionExt>(stats, data)
{
}

std::optional<uint32_t> StatsPeerConnectionImpl::dataChannelsOpened() const
{
    if (_stats) {
        return _stats->data_channels_opened;
    }
    return {};
}

std::optional<uint32_t> StatsPeerConnectionImpl::dataChannelsClosed() const
{
    if (_stats) {
        return _stats->data_channels_closed;
    }
    return {};
}

} // namespace LiveKitCpp
