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
#pragma once // RemoteTrack.h
#include "Track.h"
#include <api/rtp_receiver_interface.h>

namespace LiveKitCpp
{

class TrackManager;

class RemoteTrack : public Track
{
public:
    RemoteTrack(TrackManager* manager, rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver);
    static std::string sid(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver);
    // impl. of Track
    bool remote() const noexcept final { return true; }
    TrackSource source() const final { return TrackSource::Unknown; }
    TrackType type() const final;
    bool live() const final;
    std::string sid() const final { return _sid; }
    void mute(bool mute) final;
    bool muted() const final;
private:
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> raw() const;
private:
    TrackManager* const _manager;
    const rtc::scoped_refptr<webrtc::RtpReceiverInterface> _receiver;
    const std::string _sid;
};

} // namespace LiveKitCpp
