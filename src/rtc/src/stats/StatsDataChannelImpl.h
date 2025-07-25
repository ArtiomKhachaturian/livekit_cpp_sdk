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
#pragma once // StatsDataChannelImpl.h
#include "livekit/rtc/stats/StatsDataChannelExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsDataChannelImpl : public StatsDataImpl<webrtc::RTCDataChannelStats, StatsDataChannelExt>
{
public:
    StatsDataChannelImpl(const webrtc::RTCDataChannelStats* stats,
                         const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::DataChannel; }
    // impl. of StatsDataChannelExt
    std::optional<std::string> label() const final;
    std::optional<std::string> protocol() const final;
    std::optional<int32_t> dataChannelIdentifier() const final;
    std::optional<std::string> state() const final;
    std::optional<uint32_t> messagesSent() const final;
    std::optional<uint64_t> bytesSent() const final;
    std::optional<uint32_t> messagesReceived() const final;
    std::optional<uint64_t> bytesReceived() const final;
};

} // namespace LiveKitCpp
