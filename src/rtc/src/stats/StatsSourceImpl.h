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
#pragma once // StatsSourceImpl.h
#include "Listeners.h"
#include <api/stats/rtc_stats_collector_callback.h>

namespace LiveKitCpp
{

class StatsListener;

class StatsSourceImpl : public webrtc::RTCStatsCollectorCallback
{
public:
    StatsSourceImpl() = default;
    // impl. of StatsSource
    void addListener(StatsListener* listener);
    void removeListener(StatsListener* listener);
    void clearListeners() { _listeners.clear(); }
    // impl. of webrtc::RTCStatsCollectorCallback
    void OnStatsDelivered(const webrtc::scoped_refptr<const webrtc::RTCStatsReport>& rtcReport) final;
private:
    Bricks::Listeners<StatsListener*> _listeners;
};

} // namespace LiveKitCpp
