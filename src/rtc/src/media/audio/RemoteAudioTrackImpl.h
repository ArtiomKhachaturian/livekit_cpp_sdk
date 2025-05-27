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
#pragma once // RemoteAudioTrackImpl.h
#include "RemoteTrackImpl.h"
#include "AudioTrackImpl.h"
#include "livekit/rtc/media/RemoteAudioTrack.h"

namespace LiveKitCpp
{

class RemoteAudioTrackImpl : public RemoteTrackImpl<AudioTrackImpl<RemoteAudioTrack>>
{
    using Base = RemoteTrackImpl<AudioTrackImpl<RemoteAudioTrack>>;
public:
    RemoteAudioTrackImpl(const TrackInfo& info,
                         const webrtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                         std::shared_ptr<AudioDeviceImpl> audioDevice,
                         const std::weak_ptr<TrackManager>& trackManager);
    // impl. of RemoteAudioTrack
    bool dtx() const final { return !info()()._disableDtx; }
    bool stereo() const final { return info()()._stereo; }
    bool red() const final { return !info()()._disableRed; }
};

} // namespace LiveKitCpp
