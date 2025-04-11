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
#pragma once // StatsPeerConnectionImpl.h
#ifdef WEBRTC_AVAILABLE
#include "stats/StatsPeerConnectionExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsPeerConnectionImpl : public StatsDataImpl<webrtc::RTCPeerConnectionStats, StatsPeerConnectionExt>
{
public:
    StatsPeerConnectionImpl(const webrtc::RTCPeerConnectionStats* stats,
                            const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::PeerConnection; }
    // impl. of StatsPeerConnectionExt
    std::optional<uint32_t> dataChannelsOpened() const final;
    std::optional<uint32_t> dataChannelsClosed() const final;
};

} // namespace LiveKitCpp
#endif
