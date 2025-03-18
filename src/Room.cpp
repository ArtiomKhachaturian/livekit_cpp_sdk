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
struct Room::Impl
{
    Impl(std::unique_ptr<Websocket::EndPoint> socket,
         PeerConnectionFactory* pcf,
         const Options& signalOptions,
         const std::shared_ptr<Bricks::Logger>& logger);
    RTCEngine _engine;
};

Room::Room(std::unique_ptr<Websocket::EndPoint> socket,
           PeerConnectionFactory* pcf,
           const Options& signalOptions,
           const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(std::make_unique<Impl>(std::move(socket), pcf, signalOptions, logger))
{
}

Room::~Room()
{
    disconnect();
}

bool Room::connect(std::string host, std::string authToken)
{
    return _impl->_engine.connect(std::move(host), std::move(authToken));
}

void Room::disconnect()
{
    _impl->_engine.disconnect();
}

void Room::setListener(RoomListener* listener)
{
    _impl->_engine.setListener(listener);
}

std::shared_ptr<LocalParticipant> Room::localParticipant() const
{
    return _impl->_engine.localParticipant();
}

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(size_t index) const
{
    return _impl->_engine.remoteParticipants().at(index);
}

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(const std::string& sid) const
{
    return _impl->_engine.remoteParticipants().at(sid);
}

size_t Room::remoteParticipantsCount() const
{
    return _impl->_engine.remoteParticipants().count();
}

Room::Impl::Impl(std::unique_ptr<Websocket::EndPoint> socket,
                 PeerConnectionFactory* pcf,
                 const Options& signalOptions,
                 const std::shared_ptr<Bricks::Logger>& logger)
    : _engine(signalOptions, pcf, std::move(socket), logger)
{
}

#else
struct Room::Impl {};
    
Room::Room(std::unique_ptr<Websocket::EndPoint>, PeerConnectionFactory*,
           const Options&, const std::shared_ptr<Bricks::Logger>&) {}

Room::~Room() {}

bool Room::connect(std::string, std::string) { return false; }

void Room::disconnect() {}

void Room::setListener(RoomListener*) {}

std::shared_ptr<LocalParticipant> Room::localParticipant() const { return {}; }

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(size_t) const { return {}; }

std::shared_ptr<RemoteParticipant> Room::remoteParticipant(const std::string&) const { return {}; }

size_t Room::remoteParticipantsCount() const { return 0U; }

#endif

} // namespace LiveKitCpp
