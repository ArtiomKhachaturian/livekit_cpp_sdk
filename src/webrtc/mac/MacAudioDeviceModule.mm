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
#include "MacAudioDeviceModule.h"
#include "MacAudioDevice.h"

namespace LiveKitCpp
{

MacAudioDeviceModule::MacAudioDeviceModule(webrtc::TaskQueueFactory* taskQueueFactory)
    : _taskQueueFactory(taskQueueFactory)
{
}

MacAudioDeviceModule::~MacAudioDeviceModule()
{
}

int32_t MacAudioDeviceModule::ActiveAudioLayer(AudioLayer* audioLayer) const
{
    if (audioLayer && _audioDevice) {
        AudioLayer activeAudio;
        const auto result = _audioDevice->ActiveAudioLayer(activeAudio);
        if (0 == result) {
            *audioLayer = activeAudio;
        }
        return result;
    }
    return -1;
}

int32_t MacAudioDeviceModule::RegisterAudioCallback(webrtc::AudioTransport* audioCallback)
{
    if (_audioDeviceBuffer) {
        return _audioDeviceBuffer->RegisterAudioCallback(audioCallback);
    }
    return -1;
}

int32_t MacAudioDeviceModule::Init()
{
    if (!_audioDevice) {
        auto audioDeviceBuffer = std::make_unique<webrtc::AudioDeviceBuffer>(_taskQueueFactory);
        auto audioDevice = std::make_unique<MacAudioDevice>();
        audioDevice->AttachAudioBuffer(audioDeviceBuffer.get());
        const auto status = audioDevice->Init();
        if (webrtc::AudioDeviceGeneric::InitStatus::OK == status) {
            _audioDeviceBuffer = std::move(audioDeviceBuffer);
            _audioDevice = std::move(audioDevice);
        }
        else {
            RTC_LOG(LS_ERROR) << "Audio device initialization failed.";
        }
    }
    return _audioDevice ? 0 : -1;
}

int32_t MacAudioDeviceModule::Terminate()
{
    if (_audioDevice) {
        const auto status = _audioDevice->Terminate();
        if (0 == status) {
            _audioDevice.reset();
            _audioDeviceBuffer.reset();
        }
        return status;
    }
    return 0;
}

bool MacAudioDeviceModule::Initialized() const
{
    return nullptr != _audioDevice.get();
}

int16_t MacAudioDeviceModule::PlayoutDevices()
{
    if (_audioDevice) {
        return _audioDevice->PlayoutDevices();
    }
    return -1;
}

int16_t MacAudioDeviceModule::RecordingDevices()
{
    if (_audioDevice) {
        return _audioDevice->RecordingDevices();
    }
    return -1;
}

int32_t MacAudioDeviceModule::PlayoutDeviceName(uint16_t index,
                                                char name[webrtc::kAdmMaxDeviceNameSize],
                                                char guid[webrtc::kAdmMaxGuidSize])
{
    if (_audioDevice) {
        return _audioDevice->PlayoutDeviceName(index, name, guid);
    }
    return -1;
}

int32_t MacAudioDeviceModule::RecordingDeviceName(uint16_t index,
                                                  char name[webrtc::kAdmMaxDeviceNameSize],
                                                  char guid[webrtc::kAdmMaxGuidSize])
{
    if (_audioDevice) {
        return _audioDevice->RecordingDeviceName(index, name, guid);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetPlayoutDevice(uint16_t index)
{
    if (_audioDevice) {
        return _audioDevice->SetPlayoutDevice(index);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetPlayoutDevice(WindowsDeviceType device)
{
    if (_audioDevice) {
        return _audioDevice->SetPlayoutDevice(device);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetRecordingDevice(uint16_t index)
{
    if (_audioDevice) {
        return _audioDevice->SetRecordingDevice(index);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetRecordingDevice(WindowsDeviceType device)
{
    if (_audioDevice) {
        return _audioDevice->SetRecordingDevice(device);
    }
    return -1;
}

int32_t MacAudioDeviceModule::PlayoutIsAvailable(bool* available)
{
    if (available && _audioDevice) {
        return _audioDevice->PlayoutIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::InitPlayout()
{
    if (_audioDevice) {
        return _audioDevice->InitPlayout();
    }
    return -1;
}

bool MacAudioDeviceModule::PlayoutIsInitialized() const
{
    return _audioDevice && _audioDevice->PlayoutIsInitialized();
}

int32_t MacAudioDeviceModule::RecordingIsAvailable(bool* available)
{
    if (available && _audioDevice) {
        return _audioDevice->RecordingIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::InitRecording()
{
    if (_audioDevice) {
        return _audioDevice->InitRecording();
    }
    return -1;
}

bool MacAudioDeviceModule::RecordingIsInitialized() const
{
    return _audioDevice && _audioDevice->RecordingIsInitialized();
}

int32_t MacAudioDeviceModule::StartPlayout()
{
    if (_audioDevice) {
        return _audioDevice->StartPlayout();
    }
    return -1;
}

int32_t MacAudioDeviceModule::StopPlayout()
{
    if (_audioDevice) {
        return _audioDevice->StopPlayout();
    }
    return -1;
}

bool MacAudioDeviceModule::Playing() const
{
    return _audioDevice && _audioDevice->Playing();
}

int32_t MacAudioDeviceModule::StartRecording()
{
    if (_audioDevice) {
        return _audioDevice->StartRecording();
    }
    return -1;
}

int32_t MacAudioDeviceModule::StopRecording()
{
    if (_audioDevice) {
        return _audioDevice->StopRecording();
    }
    return -1;
}

bool MacAudioDeviceModule::Recording() const
{
    return _audioDevice && _audioDevice->Recording();
}

int32_t MacAudioDeviceModule::InitSpeaker()
{
    if (_audioDevice) {
        return _audioDevice->InitSpeaker();
    }
    return -1;
}

bool MacAudioDeviceModule::SpeakerIsInitialized() const
{
    return _audioDevice && _audioDevice->SpeakerIsInitialized();
}

int32_t MacAudioDeviceModule::InitMicrophone()
{
    if (_audioDevice) {
        return _audioDevice->InitMicrophone();
    }
    return -1;
}

bool MacAudioDeviceModule::MicrophoneIsInitialized() const
{
    return _audioDevice && _audioDevice->MicrophoneIsInitialized();
}

int32_t MacAudioDeviceModule::SpeakerVolumeIsAvailable(bool* available)
{
    if (available && _audioDevice) {
        return _audioDevice->SpeakerVolumeIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetSpeakerVolume(uint32_t volume)
{
    if (_audioDevice) {
        return _audioDevice->SetSpeakerVolume(volume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SpeakerVolume(uint32_t* volume) const
{
    if (volume && _audioDevice) {
        return _audioDevice->SpeakerVolume(*volume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MaxSpeakerVolume(uint32_t* maxVolume) const
{
    if (maxVolume && _audioDevice) {
        return _audioDevice->MaxSpeakerVolume(*maxVolume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MinSpeakerVolume(uint32_t* minVolume) const
{
    if (minVolume && _audioDevice) {
        return _audioDevice->MinSpeakerVolume(*minVolume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MicrophoneVolumeIsAvailable(bool* available)
{
    if (available && _audioDevice) {
        return _audioDevice->MicrophoneVolumeIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetMicrophoneVolume(uint32_t volume)
{
    if (_audioDevice) {
        return _audioDevice->SetMicrophoneVolume(volume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MicrophoneVolume(uint32_t* volume) const
{
    if (volume && _audioDevice) {
        return _audioDevice->MicrophoneVolume(*volume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MaxMicrophoneVolume(uint32_t* maxVolume) const
{
    if (maxVolume && _audioDevice) {
        return _audioDevice->MaxMicrophoneVolume(*maxVolume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MinMicrophoneVolume(uint32_t* minVolume) const
{
    if (minVolume && _audioDevice) {
        return _audioDevice->MinMicrophoneVolume(*minVolume);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SpeakerMuteIsAvailable(bool* available)
{
    if (available && _audioDevice) {
        return _audioDevice->SpeakerMuteIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetSpeakerMute(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->SetSpeakerMute(enable);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SpeakerMute(bool* enabled) const
{
    if (enabled && _audioDevice) {
        return _audioDevice->SpeakerMute(*enabled);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MicrophoneMuteIsAvailable(bool* available)
{
    if (available && _audioDevice) {
        return _audioDevice->MicrophoneMuteIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetMicrophoneMute(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->SetMicrophoneMute(enable);
    }
    return -1;
}

int32_t MacAudioDeviceModule::MicrophoneMute(bool* enabled) const
{
    if (enabled && _audioDevice) {
        return _audioDevice->MicrophoneMute(*enabled);
    }
    return -1;
}

int32_t MacAudioDeviceModule::StereoPlayoutIsAvailable(bool* available) const
{
    if (available && _audioDevice) {
        return _audioDevice->StereoPlayoutIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetStereoPlayout(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->SetStereoPlayout(enable);
    }
    return -1;
}

int32_t MacAudioDeviceModule::StereoPlayout(bool* enabled) const
{
    if (enabled && _audioDevice) {
        return _audioDevice->StereoPlayout(*enabled);
    }
    return -1;
}

int32_t MacAudioDeviceModule::StereoRecordingIsAvailable(bool* available) const
{
    if (available && _audioDevice) {
        return _audioDevice->StereoRecordingIsAvailable(*available);
    }
    return -1;
}

int32_t MacAudioDeviceModule::SetStereoRecording(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->SetStereoRecording(enable);
    }
    return -1;
}

int32_t MacAudioDeviceModule::StereoRecording(bool* enabled) const
{
    if (enabled && _audioDevice) {
        return _audioDevice->StereoRecording(*enabled);
    }
    return -1;
}

int32_t MacAudioDeviceModule::PlayoutDelay(uint16_t* delayMS) const
{
    if (delayMS && _audioDevice) {
        return _audioDevice->PlayoutDelay(*delayMS);
    }
    return -1;
}

bool MacAudioDeviceModule::BuiltInAECIsAvailable() const
{
    return true;
}

int32_t MacAudioDeviceModule::EnableBuiltInAEC(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->EnableBuiltInAEC(enable);
    }
    return -1;
}

bool MacAudioDeviceModule::BuiltInAGCIsAvailable() const
{
    return true;
}

int32_t MacAudioDeviceModule::EnableBuiltInAGC(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->EnableBuiltInAGC(enable);
    }
    return -1;
}

bool MacAudioDeviceModule::BuiltInNSIsAvailable() const
{
    return true;
}

int32_t MacAudioDeviceModule::EnableBuiltInNS(bool enable)
{
    if (_audioDevice) {
        return _audioDevice->EnableBuiltInNS(enable);
    }
    return -1;
}

} // namespace LiveKitCpp
