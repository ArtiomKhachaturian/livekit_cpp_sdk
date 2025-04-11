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
#pragma once // TrackManager.h
#include "livekit/signaling/sfu/EncryptionType.h"
#include <api/scoped_refptr.h>
#include <optional>
#include <string>

namespace webrtc {
class RtpReceiverInterface;
class RtpSenderInterface;
class RTCStatsCollectorCallback;
}

namespace LiveKitCpp
{

class TrackManager
{
public:
    virtual void notifyAboutMuteChanges(const std::string& trackSid, bool muted) = 0;
    virtual std::optional<bool> stereoRecording() const = 0;
    virtual EncryptionType localEncryptionType() const = 0;
    virtual void queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                            const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const = 0;
    virtual void queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                            const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const = 0;
protected:
    virtual ~TrackManager() = default;
};

} // namespace LiveKitCpp
