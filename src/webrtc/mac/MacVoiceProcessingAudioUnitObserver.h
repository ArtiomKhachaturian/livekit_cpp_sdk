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
#pragma once
#ifdef __APPLE__
#include <AudioUnit/AudioUnit.h>

namespace LiveKitCpp
{

class MacVoiceProcessingAudioUnitObserver 
{
public:
    // Callback function called on a real-time priority I/O thread from the audio
    // unit. This method is used to signal that recorded audio is available.
    virtual OSStatus OnDeliverRecordedData(AudioUnitRenderActionFlags* flags,
                                           const AudioTimeStamp* timestamp,
                                           UInt32 busNumber,
                                           UInt32 numFrames) = 0;

    // Callback function called on a real-time priority I/O thread from the audio
    // unit. This method is used to provide audio samples to the audio unit.
    virtual OSStatus OnGetPlayoutData(AudioUnitRenderActionFlags* flags,
                                      const AudioTimeStamp* timestamp,
                                      UInt32 busNumber,
                                      UInt32 numFrames,
                                      AudioBufferList* ioData) = 0;

protected:
    ~MacVoiceProcessingAudioUnitObserver() = default;
};

} // namespace LiveKitCpp
#endif
