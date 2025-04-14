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
#include "NonBindedRtpReceivers.h"
#include "Utils.h"
#include <iostream>

namespace LiveKitCpp
{

bool NonBindedRtpReceivers::add(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    if (receiver) {
        auto id = receiver->id();
        if (!id.empty()) {
            LOCK_WRITE_SAFE_OBJ(_receivers);
            _receivers->insert(std::make_pair(std::move(id), std::move(receiver)));
            return true;
        }
    }
    return false;
}

rtc::scoped_refptr<webrtc::RtpReceiverInterface> NonBindedRtpReceivers::take(const std::string& id)
{
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver;
    if (!id.empty()) {
        LOCK_WRITE_SAFE_OBJ(_receivers);
        const auto it = _receivers->find(id);
        if (it != _receivers->end()) {
            receiver = std::move(it->second);
            _receivers->erase(it);
        }
    }
    return receiver;
}

} // namespace LiveKitCpp
