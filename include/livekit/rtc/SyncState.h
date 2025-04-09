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
#pragma once // SyncState.h
#include "livekit/rtc/DataChannelInfo.h"
#include "livekit/rtc/SessionDescription.h"
#include "livekit/rtc/TrackPublishedResponse.h"
#include "livekit/rtc/UpdateSubscription.h"
#include <string>
#include <vector>

namespace LiveKitCpp
{

// sync client's subscribe state to server during reconnect
struct SyncState
{
    // last subscribe answer before reconnecting
    SessionDescription _answer;
    UpdateSubscription _subscription;
    std::vector<TrackPublishedResponse> _publishTracks;
    std::vector<DataChannelInfo> _dataChannels;
    // last received server side offer before reconnecting
    SessionDescription _offer;
    std::vector<std::string> _trackSidsDisabled;
};

} // namespace LiveKitCpp
