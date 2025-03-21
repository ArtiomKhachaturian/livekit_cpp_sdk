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
#include "Loggable.h"
#include "Listeners.h"
#include "MediaDevice.h"
#include "SafeScopedRefPtr.h"
#include <api/function_view.h>
#include <modules/audio_device/include/audio_device.h> //AudioDeviceModule
#include <type_traits>

namespace rtc {
class Thread;
}

namespace LiveKitCpp
{

class AudioDeviceModuleListener;

class AudioDeviceProxyModule : public Bricks::LoggableS<webrtc::AudioDeviceModule>
{
    template<bool recording> class ScopedAudioBlocker;
public:
    static webrtc::scoped_refptr<AudioDeviceProxyModule>
        create(rtc::Thread* workingThread,
               webrtc::TaskQueueFactory* taskQueueFactory,
               const std::shared_ptr<Bricks::Logger>& logger = {});
    ~AudioDeviceProxyModule() override;

    // impl. of webrtc::AudioDeviceModule
    // Retrieve the currently utilized audio layer
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const final;

    // Full-duplex transportation of PCM audio
    int32_t RegisterAudioCallback(webrtc::AudioTransport* audioCallback) final;

    // Main initialization and termination
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
    //int32_t SpeakerVolumeStepSize(uint16_t* stepSize) const final;

    // Microphone volume controls
    int32_t MicrophoneVolumeIsAvailable(bool* available) final;
    int32_t SetMicrophoneVolume(uint32_t volume) final;
    int32_t MicrophoneVolume(uint32_t* volume) const final;
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const final;
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const final;
    //int32_t MicrophoneVolumeStepSize(uint16_t* stepSize) const final;

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
    bool BuiltInAGCIsAvailable() const final;
    bool BuiltInNSIsAvailable() const final;

    int32_t EnableBuiltInAEC(bool enable) final;
    int32_t EnableBuiltInAGC(bool enable) final;
    int32_t EnableBuiltInNS(bool enable) final;
    
    int32_t GetPlayoutUnderrunCount() const final;
    std::optional<Stats> GetStats() const final;

    void close();
    
    void addListener(AudioDeviceModuleListener* listener);
    void removeListener(AudioDeviceModuleListener* listener);
    // selection management
    MediaDevice defaultRecordingDevice() const;
    MediaDevice defaultPlayoutDevice() const;
    bool setRecordingDevice(const MediaDevice& device);
    MediaDevice recordingDevice() const { return _recordingDev(); }
    bool setPlayoutDevice(const MediaDevice& device);
    MediaDevice playoutDevice() const { return _playoutDev(); }
    std::vector<MediaDevice> recordingDevices() const;
    std::vector<MediaDevice> playoutDevices() const;
protected:
    AudioDeviceProxyModule(rtc::Thread* thread,
                           rtc::scoped_refptr<webrtc::AudioDeviceModule> impl,
                           const std::shared_ptr<Bricks::Logger>& logger);
    // final of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    static std::optional<MediaDevice>
        get(bool recording, uint16_t ndx,
            const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    static std::optional<MediaDevice>
        get(bool recording, WindowsDeviceType device,
            const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    std::vector<MediaDevice> enumerate(bool recording) const;
    MediaDevice defaultDevice(bool recording) const;
    void requestRecordingAuthorizationStatus() const;
    // and trigger signals if index or type was changed
    void setRecordingDevice(uint16_t ndx,
                            const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    void setRecordingDevice(WindowsDeviceType device,
                            const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    void setPlayoutDevice(uint16_t ndx,
                          const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    void setPlayoutDevice(WindowsDeviceType device,
                          const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm);
    void changeRecordingDevice(const MediaDevice& device);
    void changePlayoutDevice(const MediaDevice& device);
    
    template <typename Handler>
    void threadInvoke(Handler handler) const;
    template <typename Handler, typename R = std::invoke_result_t<Handler>>
    R threadInvokeR(Handler handler, R defaultVal = {}) const;
    template <typename Handler>
    int32_t threadInvokeI32(Handler handler, int32_t defaultVal = -1) const;
private:
    rtc::Thread* const _thread;
    SafeScopedRefPtr<webrtc::AudioDeviceModule> _impl;
    Bricks::Listeners<AudioDeviceModuleListener*> _listeners;
    Bricks::SafeObj<MediaDevice> _recordingDev;
    Bricks::SafeObj<MediaDevice> _playoutDev;
};

} // namespace LiveKitCpp
