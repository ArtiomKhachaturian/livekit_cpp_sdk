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
#pragma once // AudioSourceImpl.h
#include "AsyncMediaSourceImpl.h"

namespace LiveKitCpp
{

class AudioSourceImpl : public AsyncMediaSourceImpl
{
public:
    AudioSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                    const std::shared_ptr<Bricks::Logger>& logger = {},
                    bool liveImmediately = false);
    void setVolume(double volume);
    void registerAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer);
    void UnregisterAudioObserver(webrtc::AudioSourceInterface::AudioObserver* observer);
    void addSink(webrtc::AudioTrackSinkInterface*  sink);
    void removeSink(webrtc::AudioTrackSinkInterface* sink);
protected:
    virtual void changeVolume(double volume);
    void sendData(const void* audioData, int bitsPerSample, int sampleRate,
                  size_t numberOfChannels, size_t numberOfFrames) const;
protected:
    // override of AsyncMediaSourceImpl
    void onEnabled(bool enabled) final;
private:
    AsyncListeners<webrtc::AudioSourceInterface::AudioObserver*> _observers;
    Bricks::Listeners<webrtc::AudioTrackSinkInterface*> _sinks;
};

} // namespace LiveKitCpp
