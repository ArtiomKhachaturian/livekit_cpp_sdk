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
#include "RtcInitializer.h"
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>
#include <rtc_base/crypto_random.h>
#include <rtc_base/time_utils.h>
#include <system_wrappers/include/field_trial.h>
#include <libyuv/cpu_id.h>

namespace LiveKitCpp
{

RtcInitializer::RtcInitializer()
    : _sslInitialized(rtc::InitializeSSL())
{
    if (_sslInitialized) {
        rtc::LogMessage::LogToDebug(rtc::LS_NONE);
        rtc::LogMessage::LogTimestamps(false);
        rtc::LogMessage::SetLogToStderr(false);
        libyuv::MaskCpuFlags(-1); // to enable all cpu specific optimizations
        rtc::InitRandom(static_cast<int>(rtc::Time32()));
        webrtc::field_trial::InitFieldTrialsFromString("WebRTC-SupportVP9SVC/EnabledByFlag_3SL3TL/");
    }
}

RtcInitializer::~RtcInitializer()
{
    if (_sslInitialized) {
        rtc::CleanupSSL();
    }
}

} // namespace LiveKitCpp
