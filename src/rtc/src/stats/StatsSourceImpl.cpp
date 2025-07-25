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
#include "StatsSourceImpl.h"
#include "livekit/rtc/stats/StatsListener.h"
#include "StatsReportData.h"

namespace LiveKitCpp
{

void StatsSourceImpl::addListener(StatsListener* listener)
{
    _listeners.add(listener);
}

void StatsSourceImpl::removeListener(StatsListener* listener)
{
    _listeners.remove(listener);
}

void StatsSourceImpl::OnStatsDelivered(const webrtc::scoped_refptr<const webrtc::RTCStatsReport>& rtcReport)
{
    if (rtcReport && _listeners && rtcReport->size()) {
        const StatsReport report(new StatsReportData{rtcReport});
        _listeners.invoke(&StatsListener::onStats, report);
    }
}

} // namespace LiveKitCpp
