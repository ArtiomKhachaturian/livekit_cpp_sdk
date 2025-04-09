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
#pragma once // LeaveRequest.h
#include "rtc/DisconnectReason.h"
#include "rtc/LeaveRequestAction.h"
#include "rtc/RegionSettings.h"

namespace LiveKitCpp
{

// Immediately terminate session
struct LeaveRequest
{
    // sent when server initiates the disconnect due to server-restart
    // indicates clients should attempt full-reconnect sequence
    // NOTE: `_canReconnect` obsoleted by `action` starting in protocol version 13
    bool _canReconnect = {};
    DisconnectReason _reason = {};
    LeaveRequestAction _action = {};
    RegionSettings _regions = {};
};

} // namespace LiveKitCpp
