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
#pragma once // LiveKitService.h
#include "LiveKitClientExport.h"
#include "LiveKitServiceState.h"
#include "Options.h"
#include <memory>

namespace Websocket {
class Factory;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class LiveKitRoom;
enum class NetworkType;

class LIVEKIT_CLIENT_API LiveKitService
{
    class Impl;
public:
    LiveKitService(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                   const std::shared_ptr<Bricks::Logger>& logger = {},
                   bool logWebrtcEvents = false);
    ~LiveKitService();
    LiveKitServiceState state() const;
    [[deprecated("use 'makeRoomS' or 'makeRoomU' methods for better safety & control")]]
    LiveKitRoom* makeRoom(const Options& signalOptions = {}) const;
    std::shared_ptr<LiveKitRoom> makeRoomS(const Options& signalOptions = {}) const;
    std::unique_ptr<LiveKitRoom> makeRoomU(const Options& signalOptions = {}) const;
    static NetworkType activeNetworkType();
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
