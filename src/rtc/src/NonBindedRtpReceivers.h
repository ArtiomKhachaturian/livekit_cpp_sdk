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
#pragma once // NonBindedRtpReceivers.h
#include "SafeObj.h"
#include "RtpReceiversStorage.h"
#include <unordered_map>

namespace LiveKitCpp
{

class NonBindedRtpReceivers : public RtpReceiversStorage
{
    // key is webrtc::RtpReceiverInterface::id/sid
    using ReceiversMap = std::unordered_map<std::string, webrtc::scoped_refptr<webrtc::RtpReceiverInterface>>;
public:
    bool add(std::string trackSid, webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver);
    void clear() { _receivers({}); }
    // impl. of RtpReceiversStorage
    webrtc::scoped_refptr<webrtc::RtpReceiverInterface> take(const std::string& trackSid) final;
private:
    Bricks::SafeObj<ReceiversMap> _receivers;
};
	
}
