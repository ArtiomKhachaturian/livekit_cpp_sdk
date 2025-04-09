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
#pragma once // SimulateScenario.h
#include "livekit/rtc/CandidateProtocol.h"
#include <cstdint>

namespace LiveKitCpp
{

// Simulate conditions, for client validations
struct SimulateScenario
{
    enum class Case
    {
        NotSet = 0,
        SpeakerUpdate = 1,
        NodeFailure = 2,
        Migration = 3,
        ServerLeave = 4,
        SwitchCandidateProtocol = 5,
        SubscriberBandwidth = 6,
        DisconnectSignalOnResume = 7,
        DisconnectSignalOnResumeNoMessages = 8,
        LeaveRequestFullReconnect = 9,
    };
    
    union /*scenario*/
    {
        // simulate N seconds of speaker activity
        int32_t _speakerUpdate;
        // simulate local node failure
        bool _nodeFailure;
        // simulate migration
        bool _migration;
        // server to send leave
        bool _serverLeave;
        // switch candidate protocol to tcp
        CandidateProtocol _switchCandidateProtocol;
        // maximum bandwidth for subscribers, in bps
        // when zero, clears artificial bandwidth limit
        int64_t _subscriberBandwidth;
        // disconnect signal on resume
        bool _disconnectSignalOnResume;
        // disconnect signal on resume before sending any messages from server
        bool _disconnectSignalOnResumeNoMessages;
        // full reconnect leave request
        bool _leaveRequestFullReconnect;
    } _scenario = {};
    Case _case = {};
};

} // namespace LiveKitCpp
