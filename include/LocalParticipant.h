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
#pragma once // LocalParticipant.h
#include "LiveKitClientExport.h"
#include "AudioTrack.h"
#include "CameraTrack.h"
#include "Participant.h"
#include <string>
#include <vector>

namespace Bricks {
class Blob;
}

namespace LiveKitCpp
{

class ParticipantListener;

class LocalParticipant : public Participant
{
public:
    virtual void addListener(ParticipantListener* listener) = 0;
    virtual void removeListener(ParticipantListener* listener) = 0;
    virtual CameraTrack& camera() = 0;
    virtual const CameraTrack& camera() const = 0;
    virtual AudioTrack& microphone() = 0;
    virtual const AudioTrack& microphone() const = 0;
    /**
      * Publish a new data payload to the room. Data will be forwarded to each
      * participant in the room if the destination field in publishOptions is empty
      *
      * @param data Uint8Array of the payload. To send string data, use TextEncoder.encode
      * @param options optionally specify a `reliable`, `topic` and `destination`
      */
    /**
      * whether to send this as reliable or lossy.
      * For data that you need delivery guarantee (such as chat messages), use Reliable.
      * For data that should arrive as quickly as possible, but you are ok with dropped
      * packets, use Lossy.
      */
    /**
      * the identities of participants who will receive the message, will be sent to every one if empty
      */
    /** the topic under which the message gets published */
    virtual bool publishData(std::string payload,
                             bool reliable = false,
                             const std::vector<std::string>& destinationIdentities = {},
                             std::string topic = {}) = 0;
    LIVEKIT_CLIENT_API bool publishData(const Bricks::Blob& payload,
                                        bool reliable = false,
                                        const std::vector<std::string>& destinationIdentities = {},
                                        std::string topic = {});
};

} // namespace LiveKitCpp
