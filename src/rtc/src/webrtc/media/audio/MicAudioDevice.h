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
#pragma once // MicAudioDevice.h
#include "AudioDeviceImpl.h"
#include "ListenedAudio.h"
#include <memory>

namespace webrtc {
class TaskQueueBase;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AdmProxyFacade;
class PeerConnectionFactory;
struct AudioRecordingOptions;

class MicAudioDevice : public AudioDeviceImpl
{
public:
    static std::shared_ptr<MicAudioDevice> create(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                                  const AudioRecordingOptions& options,
                                                  std::weak_ptr<AdmProxyFacade> admProxy,
                                                  const std::shared_ptr<Bricks::Logger>& logger = {});
    static std::shared_ptr<MicAudioDevice> create(const PeerConnectionFactory* pcf,
                                                  const AudioRecordingOptions& options,
                                                  const std::shared_ptr<Bricks::Logger>& logger = {});
    ~MicAudioDevice() final;
private:
    MicAudioDevice(const webrtc::scoped_refptr<ListenedAudio>& track);
};

} // namespace LiveKitCpp
