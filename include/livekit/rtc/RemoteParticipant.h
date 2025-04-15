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
#pragma once // RemoteParticipant.h
#include "livekit/rtc/Participant.h"
#include "livekit/rtc/media/RemoteAudioTrack.h"
#include "livekit/rtc/media/RemoteVideoTrack.h"
#include "livekit/signaling/sfu/ParticipantState.h"

namespace LiveKitCpp
{

class RemoteParticipantListener;

class RemoteParticipant : public Participant
{
public:
    virtual void addListener(RemoteParticipantListener* listener) = 0;
    virtual void removeListener(RemoteParticipantListener* listener) = 0;
    virtual bool hasActivePublisher() const = 0;
    virtual ParticipantState state() const = 0;
    // given track by index or server ID
    virtual std::shared_ptr<RemoteAudioTrack> audioTrack(size_t index) const = 0;
    virtual std::shared_ptr<RemoteAudioTrack> audioTrack(const std::string& sid) const = 0;
    virtual std::shared_ptr<RemoteVideoTrack> videoTrack(size_t index) const = 0;
    virtual std::shared_ptr<RemoteVideoTrack> videoTrack(const std::string& sid) const = 0;
};

} // namespace LiveKitCpp
