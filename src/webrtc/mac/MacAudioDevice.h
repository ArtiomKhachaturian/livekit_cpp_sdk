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
#include "MacAudioRecordBuffer.h"
#include <api/task_queue/pending_task_safety_flag.h>
#include <modules/audio_device/audio_device_generic.h>
#include <modules/audio_device/mac/audio_mixer_manager_mac.h>
#include <CoreAudio/CoreAudio.h>
#include <atomic>
#include <memory>
#include <vector>

namespace webrtc {
class FineAudioBuffer;
}

namespace rtc {
class Thread;
}

namespace LiveKitCpp
{

class MacVoiceProcessingAudioUnit;

class MacAudioDevice : public webrtc::AudioDeviceGeneric,
                       private MacVoiceProcessingAudioUnitObserver
{
    class FineAudioBuffer;
public:
    MacAudioDevice();
    ~MacAudioDevice() final;
    // impl. of AudioDeviceGeneric
    void AttachAudioBuffer(webrtc::AudioDeviceBuffer* audioBuffer) final;
    InitStatus Init() final;
    int32_t Terminate() final;
    bool Initialized() const final;
    int32_t InitPlayout() final;
    bool PlayoutIsInitialized() const final;
    int32_t InitRecording() final;
    bool RecordingIsInitialized() const final;
    int32_t StartPlayout() final;
    int32_t StopPlayout() final;
    bool Playing() const final;
    int32_t StartRecording() final;
    int32_t StopRecording() final;
    bool Recording() const final;
    // These methods returns hard-coded delay values and not dynamic delay
    // estimates. The reason is that iOS supports a built-in AEC and the WebRTC
    // AEC will always be disabled in the Libjingle layer to avoid running two
    // AEC implementations at the same time. And, it saves resources to avoid
    // updating these delay values continuously.
    // TODO(henrika): it would be possible to mark these two methods as not
    // implemented since they are only called for A/V-sync purposes today and
    // A/V-sync is not supported on iOS. However, we avoid adding error messages
    // the log by using these dummy implementations instead.
    int32_t PlayoutDelay(uint16_t& delayMS) const final;
    // See audio_device_not_implemented.cc for trivial implementations.
    int32_t ActiveAudioLayer(webrtc::AudioDeviceModule::AudioLayer& audioLayer) const final;
    int32_t PlayoutIsAvailable(bool& available) final;
    int32_t RecordingIsAvailable(bool& available) final;
    int16_t PlayoutDevices() final;
    int16_t RecordingDevices() final;
    int32_t PlayoutDeviceName(uint16_t index,
                              char name[webrtc::kAdmMaxDeviceNameSize],
                              char guid[webrtc::kAdmMaxGuidSize]) final;
    int32_t RecordingDeviceName(uint16_t index,
                                char name[webrtc::kAdmMaxDeviceNameSize],
                                char guid[webrtc::kAdmMaxGuidSize]) final;
    int32_t SetPlayoutDevice(uint16_t index) final;
    int32_t SetPlayoutDevice(webrtc::AudioDeviceModule::WindowsDeviceType device) final;
    int32_t SetRecordingDevice(uint16_t index) final;
    int32_t SetRecordingDevice(webrtc::AudioDeviceModule::WindowsDeviceType device) final;
    int32_t InitSpeaker() final;
    bool SpeakerIsInitialized() const final;
    int32_t InitMicrophone() final;
    bool MicrophoneIsInitialized() const final;
    int32_t SpeakerVolumeIsAvailable(bool& available) final;
    int32_t SetSpeakerVolume(uint32_t volume) final;
    int32_t SpeakerVolume(uint32_t& volume) const final;
    int32_t MaxSpeakerVolume(uint32_t& maxVolume) const final;
    int32_t MinSpeakerVolume(uint32_t& minVolume) const final;
    int32_t MicrophoneVolumeIsAvailable(bool& available) final;
    int32_t SetMicrophoneVolume(uint32_t volume) final;
    int32_t MicrophoneVolume(uint32_t& volume) const final;
    int32_t MaxMicrophoneVolume(uint32_t& maxVolume) const final;
    int32_t MinMicrophoneVolume(uint32_t& minVolume) const final;
    int32_t MicrophoneMuteIsAvailable(bool& available) final;
    int32_t SetMicrophoneMute(bool enable) final;
    int32_t MicrophoneMute(bool& enabled) const final;
    int32_t SpeakerMuteIsAvailable(bool& available) final;
    int32_t SetSpeakerMute(bool enable) final;
    int32_t SpeakerMute(bool& enabled) const final;
    int32_t StereoPlayoutIsAvailable(bool& available) final;
    int32_t SetStereoPlayout(bool enable) final;
    int32_t StereoPlayout(bool& enabled) const final;
    int32_t StereoRecordingIsAvailable(bool& available) final;
    int32_t SetStereoRecording(bool enable) final;
    int32_t StereoRecording(bool& enabled) const final;
    bool BuiltInAECIsAvailable() const final { return true; }
    bool BuiltInAGCIsAvailable() const final { return true; }
    bool BuiltInNSIsAvailable() const final { return true; }
    int32_t EnableBuiltInAEC(bool /*enable*/) { return 0; }
    int32_t EnableBuiltInAGC(bool /*enable*/) { return 0; }
    int32_t EnableBuiltInNS(bool /*enable*/) { return 0; }
 private:
    // in microseconds
    static int32_t GetDelayUs(bool input, UInt32 numFrames, const AudioTimeStamp* timestamp);
    // in nanoseconds
    static UInt64 GetHostTimeNs(const AudioTimeStamp* timestamp);
    static UInt64 GetCurrentHostTimeNs();
    static std::vector<AudioDeviceID> GetDevices(bool input);
    static AudioDeviceID GetDefaultDevice(bool input);
    static bool GetDeviceName(bool input, uint16_t index,
                              char name[webrtc::kAdmMaxDeviceNameSize],
                              char guid[webrtc::kAdmMaxGuidSize]);
    static void MarkPlayoutAsSilence(AudioUnitRenderActionFlags* flags,
                                     UInt32 numFrames, AudioBufferList* ioData);
    static void DuckAudioDevice(AudioDeviceID device, bool duck);
    // in milliseconds
    int32_t GetFullDelayMs(bool input) const;
    // Creates the audio unit.
    bool CreateAudioUnit();
    // Updates the audio unit state based on current state.
    void UpdateAudioUnit(bool canPlayOrRecord);
    // Activates our audio session, creates and initializes the voice-processing
    // audio unit and verifies that we got the preferred native audio parameters.
    bool InitPlayOrRecord();
    // Closes and deletes the voice-processing I/O unit.
    void ShutdownPlayOrRecord();
    // Resets thread-checkers before a call is restarted.
    void PrepareForNewStart();
    // impl. of VoiceProcessingAudioUnitObserver methods.
    OSStatus OnDeliverRecordedData(AudioUnitRenderActionFlags* flags,
                                   const AudioTimeStamp* timestamp,
                                   UInt32 busNumber,
                                   UInt32 numFrames) final;
    OSStatus OnGetPlayoutData(AudioUnitRenderActionFlags* flags,
                              const AudioTimeStamp* timestamp,
                              UInt32 busNumber,
                              UInt32 numFrames,
                              AudioBufferList* ioData) final;
private:
    static inline constexpr bool _audioDuckingEnabled = true;
    static inline constexpr UInt32 _recSamplesPerSec = 48000U;
    static inline constexpr UInt32 _playSamplesPerSec = 48000U;
    // Thread that this object is created on.
    rtc::Thread* const _thread;
    // Native I/O audio thread checker.
    webrtc::SequenceChecker _ioThreadChecker;
    // The AudioUnit used to play and record audio.
    std::unique_ptr<MacVoiceProcessingAudioUnit> _audioUnit;
    std::unique_ptr<FineAudioBuffer> _fineAudioBuffer;
    // Temporary storage for recorded data. AudioUnitRender() renders into this
    // array as soon as a frame of the desired buffer size has been recorded.
    // On real iOS devices, the size will be fixed and set once. For iOS
    // simulators, the size can vary from callback to callback and the size
    // will be changed dynamically to account for this behavior.
    MacAudioRecordBuffer _recordAudioBuffer;
    // Set to 1 when recording is active and 0 otherwise.
    std::atomic<int> _recording = 0;
    // Set to 1 when playout is active and 0 otherwise.
    std::atomic<int> _playing = 0;
    // Set to true after successful call to Init(), false otherwise.
    bool _initialized RTC_GUARDED_BY(_thread) = false;
    // input
    AudioDeviceID RTC_GUARDED_BY(_thread) _recordingDevice;
    std::atomic_bool _microphoneEnabled = true;
    // output
    AudioDeviceID RTC_GUARDED_BY(_thread) _playoutDevice;
    std::atomic_bool _speakerEnabled = true;
    // volume & mute control
    //webrtc::AudioMixerManagerMac _mixerManager;
    std::atomic<int32_t> _captureDelayUs = 0;
    std::atomic<int32_t> _renderDelayUs = 0;
};

} // namespace LiveKitCpp

#endif
