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
#include "MacVoiceProcessingAudioUnitObserver.h"
#include <CoreAudio/CoreAudio.h>

namespace LiveKitCpp
{

// Convenience class to abstract away the management of a Voice Processing
// I/O Audio Unit. The Voice Processing I/O unit has the same characteristics
// as the Remote I/O unit (supports full duplex low-latency audio input and
// output) and adds AEC for for two-way duplex communication. It also adds AGC,
// adjustment of voice-processing quality, and muting. Hence, ideal for
// VoIP applications.
class MacVoiceProcessingAudioUnit
{
public:
    // enum for state and state checking.
    enum class State : int32_t {
      // Init() should be called.
      InitRequired,
      // Audio unit created but not initialized.
      Uninitialized,
      // Initialized but not started. Equivalent to stopped.
      Initialized,
      // Initialized and started.
      Started,
    };
public:
    // Number of bytes per audio sample for 16-bit signed integer representation.
    static inline constexpr UInt32 _bytesPerSample = 2U;
    // Calls to AudioUnitInitialize() can fail if called back-to-back on different
    // ADM instances. A fall-back solution is to allow multiple sequential calls
    // with as small delay between each. This factor sets the max number of allowed
    // initialization attempts.
    static inline constexpr size_t _maxNumberOfAudioUnitInitializeAttempts = 5U;
    // A VP I/O unit's bus 1 connects to input hardware (microphone).
    static inline constexpr AudioUnitElement _inputBus = 1;
    // A VP I/O unit's bus 0 connects to output hardware (speaker).
    static inline constexpr AudioUnitElement _outputBus = 0;
    // Try to use mono to save resources. Also avoids channel format conversion
    // in the I/O audio unit. Initial tests have shown that it is possible to use
    // mono natively for built-in microphones and for BT headsets but not for
    // wired headsets. Wired headsets only support stereo as native channel format
    // but it is a low cost operation to do a format conversion to mono in the
    // audio unit. Hence, we will not hit a RTC_CHECK in
    // VerifyAudioParametersForActiveAudioSession() for a mismatch between the
    // preferred number of channels and the actual number of channels.
    static inline constexpr int _preferredNumberOfChannels = 1;
public:
    MacVoiceProcessingAudioUnit(MacVoiceProcessingAudioUnitObserver* observer);
    ~MacVoiceProcessingAudioUnit();
    // Initializes this class by creating the underlying audio unit instance.
    // Creates a Voice-Processing I/O unit and configures it for full-duplex
    // audio. The selected stream format is selected to avoid internal resampling
    // and to match the 10ms callback rate for WebRTC as well as possible.
    // Does not intialize the audio unit.
    OSStatus Init();
    OSStatus SetPlayoutDevice(AudioDeviceID device);
    OSStatus SetRecordingDevice(AudioDeviceID device);
    UInt32 GetCaptureLatencyUs() const noexcept { return _captureLatencyUs; }
    UInt32 GetRenderLatencyUs() const noexcept { return _renderLatencyUs; }
    State GetState() const noexcept { return _state; }
    bool Initialized() const noexcept { return State::Initialized == GetState(); }
    // Initializes the underlying audio unit with the given sample rate.
    OSStatus Initialize(Float64 sampleRate);
    // Starts the underlying audio unit.
    OSStatus Start();
    // Stops the underlying audio unit.
    OSStatus Stop();
    // Uninitializes the underlying audio unit.
    OSStatus Uninitialize();
    // Calls render on the underlying audio unit.
    OSStatus Render(AudioUnitRenderActionFlags* flags, const AudioTimeStamp* timestamp,
                    UInt32 outputBusNumber, UInt32 numFrames, AudioBufferList* ioData);
private:
    // The C API used to set callbacks requires static functions. When these are
    // called, they will invoke the relevant instance method by casting
    // in_ref_con to VoiceProcessingAudioUnit*.
    static OSStatus OnGetPlayoutData(void* inRefCon,
                                     AudioUnitRenderActionFlags* flags,
                                     const AudioTimeStamp* timestamp,
                                     UInt32 busNumber, UInt32 numFrames,
                                     AudioBufferList* ioData);
    static OSStatus OnDeliverRecordedData(void* inRefCon,
                                          AudioUnitRenderActionFlags* flags,
                                          const AudioTimeStamp* timestamp,
                                          UInt32 busNumber, UInt32 numFrames,
                                          AudioBufferList* ioData);
    template <bool inputBus, typename T>
    OSStatus GetAudioUnitProperty(AudioUnitPropertyID propId, T& value,
                                  AudioUnitScope scope = kAudioUnitScope_Global) const;
    template <bool inputBus, typename T>
    OSStatus SetAudioUnitProperty(AudioUnitPropertyID propId, const T& value,
                                  AudioUnitScope scope = kAudioUnitScope_Global);
    template <bool inputBus>
    OSStatus GetAudioUnitLatency(UInt32& latencyUs) const;
    // Returns the automatic gain control (AGC) state on the processed microphone
    // signal. Should be on by default for Voice Processing audio units.
    OSStatus GetAGCState(UInt32& enabled) const;
    template <bool inputBus>
    OSStatus SetCurrentDevice(AudioDeviceID device);
    // Returns the predetermined format with a specific sample rate. See
    // implementation file for details on format.
    AudioStreamBasicDescription GetFormat(Float64 sampleRate) const;
    // Deletes the underlying audio unit.
    void DisposeAudioUnit();
private:
    MacVoiceProcessingAudioUnitObserver* const _observer;
    AudioUnit _vpioUnit = nullptr;
    State _state = State::InitRequired;
    UInt32 _captureLatencyUs = 0U;
    UInt32 _renderLatencyUs = 0U;
};

} // namespace LiveKitCpp
#endif
