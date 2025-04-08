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
#pragma once // AudioDeviceImpl.h
#include "media/AudioDevice.h"
#include "MediaDeviceImpl.h"
#include "AudioSinks.h"

namespace LiveKitCpp
{

class AudioDeviceImpl : public MediaDeviceImpl<webrtc::AudioTrackInterface, AudioDevice>
{
    using Base = MediaDeviceImpl<webrtc::AudioTrackInterface, AudioDevice>;
public:
    AudioDeviceImpl(webrtc::scoped_refptr<webrtc::AudioTrackInterface> track);
    ~AudioDeviceImpl() override;
    // impl. of MediaDevice
    bool audio() const final { return true; }
    // impl. of AudioDevice
    void addSink(AudioSink* sink) final;
    void removeSink(AudioSink* sink) final;
    void setVolume(double volume) final;
    std::optional<int> signalLevel() const final;
private:
    AudioSinks _sinks;
};

} // namespace LiveKitCpp
