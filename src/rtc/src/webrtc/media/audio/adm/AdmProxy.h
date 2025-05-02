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
#pragma once // AdmProxyModule.h
#include "AdmProxyTypedefs.h"
#include "AdmProxyState.h"
#include "AsyncListeners.h"
#include "SafeScopedRefPtr.h"
#include "SafeObjAliases.h"
#include "livekit/rtc/media/MediaDeviceInfo.h"
#include <api/function_view.h>
#include <modules/audio_device/include/audio_device.h> //AudioDeviceModule
#include <type_traits>

namespace webrtc {
class AudioTrackSinkInterface;
class Thread;
}

namespace LiveKitCpp
{

class AdmProxyListener;
class AdmProxyTransport;

class AdmProxy : public webrtc::AudioDeviceModule
{
    class ProxyTransport;
public:
    ~AdmProxy() override;
    static webrtc::scoped_refptr<AdmProxy>
        create(const std::shared_ptr<webrtc::Thread>& workingThread,
               const std::shared_ptr<webrtc::TaskQueueBase>& signalingQueue,
               webrtc::TaskQueueFactory* taskQueueFactory);
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
    int32_t SetSpeakerMute(bool mute) final;
    int32_t SpeakerMute(bool* muted) const final;

    // Microphone mute control
    int32_t MicrophoneMuteIsAvailable(bool* available) final;
    int32_t SetMicrophoneMute(bool mute) final;
    int32_t MicrophoneMute(bool* muted) const final;

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
    
    const AdmProxyState& recordingState() const noexcept { return _recState; }
    const AdmProxyState& playoutState() const noexcept { return _playState; }
    void close();
    void registerRecordingSink(webrtc::AudioTrackSinkInterface* sink, bool reg);
    void registerRecordingListener(AdmProxyListener* l, bool reg);
    void registerPlayoutListener(AdmProxyListener* l, bool reg);
    // selection management
    MediaDeviceInfo defaultRecordingDevice() const;
    MediaDeviceInfo defaultPlayoutDevice() const;
    bool setRecordingDevice(const MediaDeviceInfo& info);
    bool setPlayoutDevice(const MediaDeviceInfo& info);
    std::vector<MediaDeviceInfo> recordingDevices() const;
    std::vector<MediaDeviceInfo> playoutDevices() const;
protected:
    AdmProxy(const std::shared_ptr<webrtc::Thread>& workingThread,
             const std::shared_ptr<webrtc::TaskQueueBase>& signalingQueue,
             AdmPtr impl);
private:
    static std::optional<MediaDeviceInfo> get(bool recording, uint16_t ndx, const AdmPtr& adm);
    static std::optional<MediaDeviceInfo> get(bool recording, WindowsDeviceType type, const AdmPtr& adm);
    static std::optional<uint16_t> get(bool recording, const MediaDeviceInfo& info, const AdmPtr& adm);
    static AdmPtr defaultAdm(webrtc::TaskQueueFactory* taskQueueFactory);
    std::shared_ptr<webrtc::Thread> workingThread() const;
    std::vector<MediaDeviceInfo> enumerate(bool recording) const;
    MediaDeviceInfo defaultDevice(bool recording) const;
    void requestRecordingAuthorizationStatus() const;
    // and trigger signals if index or type was changed
    void setRecordingDevice(uint16_t ndx, const AdmPtr& adm);
    void setRecordingDevice(WindowsDeviceType type, const AdmPtr& adm);
    void setPlayoutDevice(uint16_t ndx, const AdmPtr& adm);
    void setPlayoutDevice(WindowsDeviceType type, const AdmPtr& adm);
    void changeRecordingDevice(const MediaDeviceInfo& info, const AdmPtr& adm);
    void changePlayoutDevice(const MediaDeviceInfo& info, const AdmPtr& adm);
    int32_t changeRecordingDevice(uint16_t index, const AdmPtr& adm);
    int32_t changePlayoutDevice(uint16_t index, const AdmPtr& adm);
    template <typename Handler>
    void threadInvoke(Handler handler) const;
    template <typename Handler, typename R = std::invoke_result_t<Handler>>
    R threadInvokeR(Handler handler, R defaultVal = {}) const;
    template <typename Handler>
    int32_t threadInvokeI32(Handler handler, int32_t defaultVal = -1) const;
private:
    static constexpr AudioLayer _layer = AudioLayer::kPlatformDefaultAudio;
    const std::weak_ptr<webrtc::Thread> _workingThread;
    SafeScopedRefPtr<webrtc::AudioDeviceModule> _impl;
    Bricks::SafeUniquePtr<AdmProxyTransport> _transport;
    AdmProxyState _recState;
    AdmProxyState _playState;
};

} // namespace LiveKitCpp
