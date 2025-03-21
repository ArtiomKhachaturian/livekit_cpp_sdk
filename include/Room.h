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
#pragma once // Room.h
#include "LiveKitClientExport.h"
#include "LocalParticipant.h"
#include "RemoteParticipant.h"
#include "RoomState.h"
#include <memory>
#include <string>
#include <vector>

namespace Websocket {
class EndPoint;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class RoomListener;
class PeerConnectionFactory;
class RTCEngine;
struct Options;

class LIVEKIT_CLIENT_API Room
{
    friend class Service;
public:
    ~Room();
    RoomState state() const;
    bool connect(std::string host, std::string authToken);
    void disconnect();
    void setListener(RoomListener* listener = nullptr);
    std::shared_ptr<LocalParticipant> localParticipant() const;
    // given participant by index or server ID
    std::shared_ptr<RemoteParticipant> remoteParticipant(size_t index) const;
    std::shared_ptr<RemoteParticipant> remoteParticipant(const std::string& sid) const;
    size_t remoteParticipantsCount() const;
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
    bool sendUserPacket(std::string payload,
                        bool reliable = false,
                        const std::vector<std::string>& destinationIdentities = {},
                        const std::string& topic = {});
    // [deleted] true to remove message
    bool sendChatMessage(std::string message, bool deleted = false);
private:
    Room(std::unique_ptr<Websocket::EndPoint> socket,
         PeerConnectionFactory* pcf,
         const Options& signalOptions,
         const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    const std::unique_ptr<RTCEngine> _engine;
};

} // namespace LiveKitCpp
