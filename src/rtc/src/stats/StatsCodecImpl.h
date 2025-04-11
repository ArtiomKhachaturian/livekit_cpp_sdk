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
#pragma once // StatsCodecImpl.h
#include "livekit/rtc/stats/StatsCodecExt.h"
#include "StatsDataImpl.h"

namespace LiveKitCpp
{

class StatsCodecImpl : public StatsDataImpl<webrtc::RTCCodecStats, StatsCodecExt>
{
public:
    StatsCodecImpl(const webrtc::RTCCodecStats* stats, const std::shared_ptr<const StatsReportData>& data);
    // override of StatsData
    StatsType type() const final { return StatsType::Codec; }
    // impl. of StatsCodecExt
    std::optional<std::string> transportId() const final;
    std::optional<uint32_t> payloadType() const final;
    std::optional<std::string> mimeType() const final;
    std::optional<uint32_t> clockRate() const final;
    std::optional<uint32_t> channels() const final;
    std::optional<std::string> sdpFmtpLine() const final;
};

} // namespace LiveKitCpp
