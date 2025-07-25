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
#pragma once // AdmScopedBlocker.h
#include "AdmProxyTypedefs.h"
#include <rtc_base/logging.h>

namespace LiveKitCpp
{

template <bool recording>
class AdmScopedBlocker
{
public:
    AdmScopedBlocker(const AdmPtr& adm);
    ~AdmScopedBlocker();
private:
    const AdmPtr _adm;
    bool _needRestart = false;
};

template <bool recording>
inline AdmScopedBlocker<recording>::AdmScopedBlocker(const AdmPtr& adm)
    : _adm(adm)
{
    if constexpr (recording) {
        if (_adm && _adm->Recording()) {
            const auto res = _adm->StopRecording();
            if (0 == res) {
                _needRestart = true;
            }
            else {
                RTC_LOG(LS_WARNING) << "recording stop failed, error code: " << res;
            }
        }
    }
    else {
        if (_adm && _adm->Playing()) {
            const auto res = _adm->StopPlayout();
            if (0 == res) {
                _needRestart = true;
            }
            else {
                RTC_LOG(LS_WARNING) << "playout stop failed, error code: " << res;
            }
        }
    }
}

template <bool recording>
inline AdmScopedBlocker<recording>::~AdmScopedBlocker()
{
    if (_needRestart) {
        if constexpr (recording) {
            if (_adm) {
                auto res = _adm->InitRecording();
                if (0 != res) {
                    RTC_LOG(LS_WARNING) << "failed to re-initialize recording, error code: " << res;
                }
                else {
                    res = _adm->StartRecording();
                    if (0 != res) {
                        RTC_LOG(LS_WARNING) << "failed to restart recording, error code: " << res;
                    }
                }
            }
        }
        else {
            if (_adm) {
                auto res = _adm->InitPlayout();
                if (0 != res) {
                    RTC_LOG(LS_WARNING) << "failed to re-initialize playout, error code: " << res;
                }
                else {
                    res = _adm->StartPlayout();
                    if (0 != res) {
                        RTC_LOG(LS_WARNING) << "failed to restart playout, error code: " << res;
                    }
                }
            }
        }
    }
}

} // namespace LiveKitCpp
