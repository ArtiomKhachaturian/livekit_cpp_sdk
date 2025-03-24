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
#include "Room.h"
#ifdef WEBRTC_AVAILABLE
#include "RTCEngine.h"
#include "PeerConnectionFactory.h"
#endif
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE
Room::Room(std::unique_ptr<Websocket::EndPoint> socket,
           PeerConnectionFactory* pcf, Options options,
           const std::shared_ptr<Bricks::Logger>& logger)
    : _engine(std::make_unique<RTCEngine>(std::move(options), pcf, std::move(socket), logger))
{
}

Room::~Room()
{
    disconnect();
}

RoomState Room::state() const
{
    return _engine->state();
}

bool Room::connect(std::string host, std::string authToken)
{
    return _engine->connect(std::move(host), std::move(authToken));
}

void Room::disconnect()
{
    _engine->disconnect();
}

void Room::setListener(RoomListener* listener)
{
    _engine->setListener(listener);
}

std::shared_ptr<LocalParticipant> Room::localParticipant() const
{
    return _engine->localParticipant();
}

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(size_t index) const
{
    return _engine->remoteParticipants().at(index);
}

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(const std::string& sid) const
{
    return _engine->remoteParticipants().at(sid);
}

size_t Room::remoteParticipantsCount() const
{
    return _engine->remoteParticipants().count();
}

bool Room::sendUserPacket(std::string payload,
                          bool reliable,
                          const std::vector<std::string>& destinationIdentities,
                          const std::string& topic)
{
    return _engine->sendUserPacket(std::move(payload), reliable, destinationIdentities, topic);
}

bool Room::sendChatMessage(std::string message, bool deleted)
{
    return _engine->sendChatMessage(std::move(message), deleted);
}
#else
struct Room::Impl {};
    
Room::Room(std::unique_ptr<Websocket::EndPoint>, PeerConnectionFactory*,
           Options, const std::shared_ptr<Bricks::Logger>&) {}

Room::~Room() {}

RoomState Room::state() const { return RoomState::TransportDisconnected; }

bool Room::connect(std::string, std::string) { return false; }

void Room::disconnect() {}

void Room::setListener(RoomListener*) {}

std::shared_ptr<LocalParticipant> Room::localParticipant() const { return {}; }

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(size_t) const { return {}; }

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(const std::string&) const { return {}; }

size_t Room::remoteParticipantsCount() const { return 0U; }

bool Room::sendUserPacket(std::string, bool, const std::vector<std::string>&,
                          const std::string&) { return false; }

bool Room::sendChatMessage(std::string, bool) { return false; }

#endif

} // namespace LiveKitCpp
