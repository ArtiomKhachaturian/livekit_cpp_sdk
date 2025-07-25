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
#pragma once // VideoScalabilityMode.h
#include "livekit/rtc/LiveKitRtcExport.h"
#include <string>

namespace LiveKitCpp
{

// https://www.w3.org/TR/webrtc-svc/#scalabilitymodes*
enum class VideoScalabilityMode
{
    Auto,
    L1T1,
    L1T2,
    L1T3,
    L2T1,
    L2T2,
    L2T3,
    L3T1,
    L3T2,
    L3T3,
    L2T1h,
    L2T2h,
    L2T3h,
    L3T1h,
    L3T2h,
    L3T3h,
    S2T1,
    S2T2,
    S2T3,
    S2T1h,
    S2T2h,
    S2T3h,
    S3T1,
    S3T2,
    S3T3,
    S3T1h,
    S3T2h,
    S3T3h,
    L2T2Key,
    L2T2KeyShift,
    L2T3Key,
    L2T3KeyShift,
    L3T1Key,
    L3T2Key,
    L3T2KeyShift,
    L3T3Key,
    L3T3KeyShift,
};

LIVEKIT_RTC_API std::string toString(VideoScalabilityMode type);

} // namespace LiveKitCpp
