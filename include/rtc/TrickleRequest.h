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
#pragma once // TrickleRequest.h
#include "rtc/SignalTarget.h"
#include <string>

namespace LiveKitCpp
{

// sent when an ICE candidate is available
struct TrickleRequest
{
    std::string _candidateInit; // this is an JSON, see comments below
    SignalTarget _target = {};
    bool _final = {};
};

/*
 structure of _candidateInit:
    - candidate (SDP)
    - sdpMid
    - sdpMLineIndex
    - usernameFragment (may contains null value)
 */

} // namespace LiveKitCpp
