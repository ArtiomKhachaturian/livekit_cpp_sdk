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
#pragma once // Service.h
#include "LiveKitClientExport.h"
#include "ServiceState.h"
#include "MediaDevice.h"
#include "MediaAuthorizationLevel.h"
#include "Options.h"
#include <memory>
#include <vector>

namespace Websocket {
class Factory;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class Room;
enum class NetworkType;

class LIVEKIT_CLIENT_API Service
{
    class Impl;
public:
    Service(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
            const std::shared_ptr<Bricks::Logger>& logger = {},
            bool logWebrtcEvents = false);
    ~Service();
    ServiceState state() const;
    [[deprecated("use 'makeRoomS' or 'makeRoomU' methods for better safety & control")]]
    Room* makeRoom(Options options = {}) const;
    std::shared_ptr<Room> makeRoomS(Options options = {}) const;
    std::unique_ptr<Room> makeRoomU(Options options = {}) const;
    // media & network devices
    static NetworkType activeNetworkType();
    static MediaAuthorizationLevel mediaAuthorizationLevel();
    void setMediaAuthorizationLevel(MediaAuthorizationLevel level);
    MediaDevice defaultRecordingCameraDevice() const;
    MediaDevice defaultRecordingAudioDevice() const;
    MediaDevice defaultPlayoutAudioDevice() const;
    // device for input from micrpohone
    bool setRecordingAudioDevice(const MediaDevice& device);
    MediaDevice recordingAudioDevice() const;
    // device for output from speakers
    bool setPlayoutAudioDevice(const MediaDevice& device);
    MediaDevice playoutAudioDevice() const;
    // enumeration
    std::vector<MediaDevice> recordingAudioDevices() const;
    std::vector<MediaDevice> playoutAudioDevices() const;
    std::vector<MediaDevice> recordingCameraDevices() const;
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
