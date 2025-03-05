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
#include <api/audio/audio_device.h>
#include <memory>

namespace webrtc {
class TaskQueueFactory;
class AudioDeviceBuffer;
}

namespace LiveKitCpp
{

class MacAudioDevice;

class MacAudioDeviceModule : public webrtc::AudioDeviceModule
{
public:
    MacAudioDeviceModule(webrtc::TaskQueueFactory* taskQueueFactory);
    ~MacAudioDeviceModule() override;
    // impl. of webrtc::AudioDeviceModule
    // Retrieve the currently utilized audio layer
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const final;
    // Full-duplex transportation of PCM audio
    int32_t RegisterAudioCallback(webrtc::AudioTransport* audioCallback) final;
    // Main initializaton and termination
    int32_t Init() final;
    int32_t Terminate() final;
    bool Initialized() const final;
    // Device enumeration
    int16_t PlayoutDevices() final;
    int16_t RecordingDevices() final;
    int32_t PlayoutDeviceName(uint16_t index,
                            char name[webrtc::kAdmMaxDeviceNameSize],
                            char guid[webrtc::kAdmMaxGuidSize]) final;
    int32_t RecordingDeviceName(uint16_t index,
                              char name[webrtc::kAdmMaxDeviceNameSize],
                              char guid[webrtc::kAdmMaxGuidSize]) final;
    // Device selection
    int32_t SetPlayoutDevice(uint16_t index) final;
    int32_t SetPlayoutDevice(WindowsDeviceType device) final;
    int32_t SetRecordingDevice(uint16_t index) final;
    int32_t SetRecordingDevice(WindowsDeviceType device) final;
    // Audio transport initialization
    int32_t PlayoutIsAvailable(bool* available) final;
    int32_t InitPlayout() final;
    bool PlayoutIsInitialized() const final;
    int32_t RecordingIsAvailable(bool* available) final;
    int32_t InitRecording() final;
    bool RecordingIsInitialized() const final;
    // Audio transport control
    int32_t StartPlayout() final;
    int32_t StopPlayout() final;
    bool Playing() const final;
    int32_t StartRecording() final;
    int32_t StopRecording() final;
    bool Recording() const final;
    // Audio mixer initialization
    int32_t InitSpeaker() final;
    bool SpeakerIsInitialized() const final;
    int32_t InitMicrophone() final;
    bool MicrophoneIsInitialized() const final;
    // Speaker volume controls
    int32_t SpeakerVolumeIsAvailable(bool* available) final;
    int32_t SetSpeakerVolume(uint32_t volume) final;
    int32_t SpeakerVolume(uint32_t* volume) const final;
    int32_t MaxSpeakerVolume(uint32_t* maxVolume) const final;
    int32_t MinSpeakerVolume(uint32_t* minVolume) const final;
    // Microphone volume controls
    int32_t MicrophoneVolumeIsAvailable(bool* available) final;
    int32_t SetMicrophoneVolume(uint32_t volume) final;
    int32_t MicrophoneVolume(uint32_t* volume) const final;
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const final;
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const final;
    // Speaker mute control
    int32_t SpeakerMuteIsAvailable(bool* available) final;
    int32_t SetSpeakerMute(bool enable) final;
    int32_t SpeakerMute(bool* enabled) const final;
    // Microphone mute control
    int32_t MicrophoneMuteIsAvailable(bool* available) final;
    int32_t SetMicrophoneMute(bool enable) final;
    int32_t MicrophoneMute(bool* enabled) const final;
    // Stereo support
    int32_t StereoPlayoutIsAvailable(bool* available) const final;
    int32_t SetStereoPlayout(bool enable) final;
    int32_t StereoPlayout(bool* enabled) const final;
    int32_t StereoRecordingIsAvailable(bool* available) const final;
    int32_t SetStereoRecording(bool enable) final;
    int32_t StereoRecording(bool* enabled) const final;
    // Delay information and control
    int32_t PlayoutDelay(uint16_t* delayMS) const final;
    bool BuiltInAECIsAvailable() const final;
    int32_t EnableBuiltInAEC(bool enable) final;
    bool BuiltInAGCIsAvailable() const final;
    int32_t EnableBuiltInAGC(bool enable) final;
    bool BuiltInNSIsAvailable() const final;
    int32_t EnableBuiltInNS(bool enable) final;
private:
    webrtc::TaskQueueFactory* const _taskQueueFactory;
    std::unique_ptr<webrtc::AudioDeviceBuffer> _audioDeviceBuffer;
    std::unique_ptr<MacAudioDevice> _audioDevice;
};

} // namespace LiveKitCpp
#endif
