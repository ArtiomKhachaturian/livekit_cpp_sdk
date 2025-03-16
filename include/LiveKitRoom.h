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
#pragma once // LiveKitRoom.h
#include "LiveKitClientExport.h"
#include "LocalParticipant.h"
#include "RemoteParticipants.h"
#include <memory>

namespace Websocket {
class EndPoint;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

struct Options;
class PeerConnectionFactory;

class LIVEKIT_CLIENT_API LiveKitRoom
{
    struct Impl;
    friend class LiveKitService;
public:
    ~LiveKitRoom();
    bool connect(std::string host, std::string authToken);
    void disconnect();
    std::shared_ptr<LocalParticipant> localParticipant() const;
    std::shared_ptr<RemoteParticipants> remoteParticipants() const;
private:
    LiveKitRoom(std::unique_ptr<Websocket::EndPoint> socket,
                PeerConnectionFactory* pcf,
                const Options& signalOptions,
                const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
