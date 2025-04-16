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
#include <api/scoped_refptr.h>
#include <api/media_types.h>
#include <optional>
#include <string>

namespace webrtc {
class RtpReceiverInterface;
class RtpSenderInterface;
class RTCStatsCollectorCallback;
}

namespace LiveKitCpp
{

class AesCgmCryptor;

class TrackManager
{
public:
    virtual webrtc::scoped_refptr<AesCgmCryptor> createCryptor(bool local,
                                                               cricket::MediaType mediaType,
                                                               std::string identity,
                                                               std::string trackId) const = 0;
    virtual void notifyAboutMuteChanges(const std::string& trackSid, bool muted) = 0;
    virtual std::optional<bool> stereoRecording() const = 0;
    virtual void queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                            const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const = 0;
    virtual void queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                            const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const = 0;
protected:
    virtual ~TrackManager() = default;
};

} // namespace LiveKitCpp
