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
#include <memory>

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
struct Options;

class LIVEKIT_CLIENT_API Room
{
    struct Impl;
    friend class Service;
public:
    ~Room();
    bool connect(std::string host, std::string authToken);
    void disconnect();
    void setListener(RoomListener* listener = nullptr);
    std::shared_ptr<LocalParticipant> localParticipant() const;
    // given participant by index or server ID
    std::shared_ptr<RemoteParticipant> remoteParticipant(size_t index) const;
    std::shared_ptr<RemoteParticipant> remoteParticipant(const std::string& sid) const;
    size_t remoteParticipantsCount() const;
private:
    Room(std::unique_ptr<Websocket::EndPoint> socket,
         PeerConnectionFactory* pcf,
         const Options& signalOptions,
         const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
