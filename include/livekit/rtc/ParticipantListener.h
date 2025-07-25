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
#pragma once // ParticipantListener.h
#include "livekit/signaling/sfu/TrackType.h"
#include "livekit/signaling/sfu/EncryptionType.h"
#include "livekit/signaling/sfu/ConnectionQuality.h"
#include <string>

namespace LiveKitCpp
{

class Participant;
enum class E2ECryptoError;

class ParticipantListener
{
public:
    virtual void onSidChanged(const Participant* /*participant*/) {}
    virtual void onIdentityChanged(const Participant* /*participant*/) {}
    virtual void onNameChanged(const Participant* /*participant*/) {}
    virtual void onMetadataChanged(const Participant* /*participant*/) {}
    virtual void onKindChanged(const Participant* /*participant*/) {}
    // audio level, 0-1.0, 1 is loudest
    // true if speaker is currently active
    virtual void onSpeakerInfoChanged(const Participant* /*participant*/,
                                      float /*level*/, bool /*active*/) {}
    virtual void onConnectionQualityChanged(const Participant* /*participant*/,
                                            ConnectionQuality /*quality*/,
                                            float /*score*/) {}
    // trackId - for local is a track ID, for remotes - SID
    virtual void onTrackCryptoError(const Participant* /*participant*/,
                                    TrackType /*type*/,
                                    EncryptionType /*encryption*/,
                                    const std::string& /*trackId*/,
                                    E2ECryptoError /*error*/) {}
protected:
    virtual ~ParticipantListener() = default;
};

} // namespace LiveKitCpp
