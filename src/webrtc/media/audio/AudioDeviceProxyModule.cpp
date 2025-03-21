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
#include "AudioDeviceProxyModule.h"
#include "AudioDevicesEnumerator.h"
#include "AudioDeviceModuleListener.h"
#include "MediaAuthorization.h"
#include "Utils.h"
#include <api/make_ref_counted.h>
#include <rtc_base/thread.h>

namespace
{

// helpers

inline void invokeInThread(rtc::Thread* to,
                           rtc::FunctionView<void()> handler) {
    if (to && !to->IsQuitting()) {
        if (to->IsCurrent()) {
            std::move(handler)();
        }
        else {
            to->BlockingCall(std::move(handler));
        }
    }
}

template <typename Handler, typename R = std::invoke_result_t<Handler>>
inline R invokeInThreadR(rtc::Thread* to, Handler handler, R defaultVal = {})
{
    if (to && !to->IsQuitting()) {
        if (to->IsCurrent()) {
            return std::move(handler)();
        }
        return to->BlockingCall<Handler, R>(std::move(handler));
    }
    return defaultVal;
}

inline rtc::scoped_refptr<webrtc::AudioDeviceModule>
    defaultAdm(webrtc::TaskQueueFactory* taskQueueFactory) {
    return webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio,
                                             taskQueueFactory);
}

inline LiveKitCpp::MediaDevice make(std::string_view name, std::string_view guid) {
    LiveKitCpp::MediaDevice device;
    device._name = name;
    device._guid = guid;
    return device;
}

#ifdef WEBRTC_MAC
// like in webrtc::AudioDeviceMac::GetDeviceName
inline constexpr const char* defaultPrefix() {
    return "default (";
}

inline bool isDefault(const char* name) {
    return name && name == std::strstr(name, defaultPrefix());
}

inline bool isDefault(const std::string_view& name) {
    return isDefault(name.data());
}
#endif

} // namespace

namespace LiveKitCpp
{

template<bool recording>
class AudioDeviceProxyModule::ScopedAudioBlocker
{
public:
    ScopedAudioBlocker(const AudioDeviceProxyModule& adm);
    ~ScopedAudioBlocker();
private:
    const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& _impl;
    const AudioDeviceProxyModule& _adm;
    bool _needRestart = false;
};

AudioDeviceProxyModule::AudioDeviceProxyModule(const std::shared_ptr<rtc::Thread>& thread,
                                               rtc::scoped_refptr<webrtc::AudioDeviceModule> impl,
                                               const std::shared_ptr<Bricks::Logger>& logger)
    :  Bricks::LoggableS<webrtc::AudioDeviceModule>(logger)
    , _impl(std::move(impl))
    , _listeners(thread)
{
    if (canLogInfo()) {
        const auto& impl = _impl.constRef();
        std::vector<std::string_view> info;
        if (impl->BuiltInAECIsAvailable()) {
            info.push_back("AEC");
        }
        if (impl->BuiltInAGCIsAvailable()) {
            info.push_back("AGC");
        }
        if (impl->BuiltInNSIsAvailable()) {
            info.push_back("NS");
        }
        if (!info.empty()) {
            logInfo("accessibility of built-in audio processing: " + join(info, ", "));
        }
    }
}

AudioDeviceProxyModule::~AudioDeviceProxyModule()
{
    close();
}

rtc::scoped_refptr<AudioDeviceProxyModule> AudioDeviceProxyModule::
    create(const std::shared_ptr<rtc::Thread>& thread,
           webrtc::TaskQueueFactory* taskQueueFactory,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (thread) {
        auto impl = invokeInThreadR(thread.get(), [taskQueueFactory](){
            return defaultAdm(taskQueueFactory);
        });
        if (impl) {
            return rtc::make_ref_counted<AudioDeviceProxyModule>(thread,
                                                                 std::move(impl),
                                                                 logger);
        }
    }
    return nullptr;
}

int32_t AudioDeviceProxyModule::ActiveAudioLayer(AudioLayer* audioLayer) const
{
    return threadInvokeI32([audioLayer](const auto& pm) {
        return pm->ActiveAudioLayer(audioLayer); });
}

// Full-duplex transportation of PCM audio
int32_t AudioDeviceProxyModule::RegisterAudioCallback(webrtc::AudioTransport* audioCallback)
{
    return threadInvokeI32([audioCallback](const auto& pm) {
        return pm->RegisterAudioCallback(audioCallback); });
}

// Main initialization and termination
int32_t AudioDeviceProxyModule::AudioDeviceProxyModule::Init()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Init(); });
}

int32_t AudioDeviceProxyModule::AudioDeviceProxyModule::Terminate()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Terminate(); });
}

bool AudioDeviceProxyModule::Initialized() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->Initialized();
    }, false);
}

// Device enumeration
int16_t AudioDeviceProxyModule::PlayoutDevices()
{
    return threadInvokeR([](const auto& pm) {
        return pm->PlayoutDevices();
    }, int16_t(0));
}

int16_t AudioDeviceProxyModule::RecordingDevices()
{
    return threadInvokeR([](const auto& pm) {
        return pm->RecordingDevices();
    }, int16_t(0));
}

int32_t AudioDeviceProxyModule::PlayoutDeviceName(uint16_t index,
                                                  char name[webrtc::kAdmMaxDeviceNameSize],
                                                  char guid[webrtc::kAdmMaxGuidSize])
{
    return threadInvokeI32([index, &name, &guid](const auto& pm) {
        return pm->PlayoutDeviceName(index, name, guid); });
}

int32_t AudioDeviceProxyModule::RecordingDeviceName(uint16_t index,
                                                    char name[webrtc::kAdmMaxDeviceNameSize],
                                                    char guid[webrtc::kAdmMaxGuidSize])
{
    return threadInvokeI32([index, &name, &guid](const auto& pm) {
        return pm->RecordingDeviceName(index, name, guid); });
}

// Device selection
int32_t AudioDeviceProxyModule::SetPlayoutDevice(uint16_t index)
{
    return threadInvokeI32([this, index](const auto& pm) {
        return changePlayoutDevice(index, pm);
    });
}

int32_t AudioDeviceProxyModule::SetPlayoutDevice(WindowsDeviceType device)
{
    return threadInvokeI32([this, device](const auto& pm) {
        int32_t result = -1;
        {
            const ScopedAudioBlocker<false> blocker(*this);
            result = pm->SetPlayoutDevice(device);
        }
        if (0 == result) {
            setPlayoutDevice(device, pm);
        }
        return result;
    });
}

int32_t AudioDeviceProxyModule::SetRecordingDevice(uint16_t index)
{
    return threadInvokeI32([this, index](const auto& pm) {
        return changeRecordingDevice(index, pm);
    });
}

int32_t AudioDeviceProxyModule::SetRecordingDevice(WindowsDeviceType device)
{
    return threadInvokeI32([this, device](const auto& pm) {
        int32_t result = -1;
        {
            const ScopedAudioBlocker<true> blocker(*this);
            result = pm->SetRecordingDevice(device);
        }
        if (0 == result) {
            setRecordingDevice(device, pm);
        }
        return result;
    });
}

// Audio transport initialization
int32_t AudioDeviceProxyModule::PlayoutIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->PlayoutIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::InitPlayout()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitPlayout(); });
}

bool AudioDeviceProxyModule::PlayoutIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->PlayoutIsInitialized(); });
}

int32_t AudioDeviceProxyModule::RecordingIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->RecordingIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::InitRecording()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitRecording(); });
}

bool AudioDeviceProxyModule::RecordingIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->RecordingIsInitialized(); });
}

// Audio transport control
int32_t AudioDeviceProxyModule::StartPlayout()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->StartPlayout(); });
}

int32_t AudioDeviceProxyModule::StopPlayout()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->StopPlayout(); });
}

bool AudioDeviceProxyModule::Playing() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Playing(); });
}

int32_t AudioDeviceProxyModule::StartRecording()
{
    return threadInvokeI32([this](const auto& pm) {
        requestRecordingAuthorizationStatus();
        return pm->StartRecording(); });
}

int32_t AudioDeviceProxyModule::StopRecording()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->StopRecording(); });
}

bool AudioDeviceProxyModule::Recording() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Recording(); });
}

// Audio mixer initialization
int32_t AudioDeviceProxyModule::InitSpeaker()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitSpeaker(); });
}

bool AudioDeviceProxyModule::SpeakerIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->SpeakerIsInitialized(); });
}

int32_t AudioDeviceProxyModule::InitMicrophone()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitMicrophone(); });
}

bool AudioDeviceProxyModule::MicrophoneIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->MicrophoneIsInitialized(); });
}

// Speaker volume controls
int32_t AudioDeviceProxyModule::SpeakerVolumeIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->SpeakerVolumeIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::SetSpeakerVolume(uint32_t volume)
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->SetSpeakerVolume(volume); });
}

int32_t AudioDeviceProxyModule::SpeakerVolume(uint32_t* volume) const
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->SpeakerVolume(volume); });
}

int32_t AudioDeviceProxyModule::MaxSpeakerVolume(uint32_t* maxVolume) const
{
    return threadInvokeI32([maxVolume](const auto& pm) {
        return pm->MaxSpeakerVolume(maxVolume); });
}

int32_t AudioDeviceProxyModule::MinSpeakerVolume(uint32_t* minVolume) const
{
    return threadInvokeI32([minVolume](const auto& pm) {
        return pm->MinSpeakerVolume(minVolume); });
}

// Microphone volume controls
int32_t AudioDeviceProxyModule::MicrophoneVolumeIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->MicrophoneVolumeIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::SetMicrophoneVolume(uint32_t volume)
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->SetMicrophoneVolume(volume); });
}

int32_t AudioDeviceProxyModule::MicrophoneVolume(uint32_t* volume) const
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->MicrophoneVolume(volume); });
}

int32_t AudioDeviceProxyModule::MaxMicrophoneVolume(uint32_t* maxVolume) const
{
    return threadInvokeI32([maxVolume](const auto& pm) {
        return pm->MaxMicrophoneVolume(maxVolume); });
}

int32_t AudioDeviceProxyModule::MinMicrophoneVolume(uint32_t* minVolume) const
{
    return threadInvokeI32([minVolume](const auto& pm) {
        return pm->MinMicrophoneVolume(minVolume); });
}

// Speaker mute control
int32_t AudioDeviceProxyModule::SpeakerMuteIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->SpeakerMuteIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::SetSpeakerMute(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->SetSpeakerMute(enable); });
}

int32_t AudioDeviceProxyModule::SpeakerMute(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->SpeakerMute(enabled); });
}

// Microphone mute control
int32_t AudioDeviceProxyModule::MicrophoneMuteIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->MicrophoneMuteIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::SetMicrophoneMute(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->SetMicrophoneMute(enable); });
}

int32_t AudioDeviceProxyModule::MicrophoneMute(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->MicrophoneMute(enabled); });
}

// Stereo support
int32_t AudioDeviceProxyModule::StereoPlayoutIsAvailable(bool* available) const
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->StereoPlayoutIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::SetStereoPlayout(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->SetStereoPlayout(enable); });
}

int32_t AudioDeviceProxyModule::StereoPlayout(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->StereoPlayout(enabled); });
}

int32_t AudioDeviceProxyModule::StereoRecordingIsAvailable(bool* available) const
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->StereoRecordingIsAvailable(available); });
}

int32_t AudioDeviceProxyModule::SetStereoRecording(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->SetStereoRecording(enable); });
}

int32_t AudioDeviceProxyModule::StereoRecording(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->StereoRecording(enabled); });
}

int32_t AudioDeviceProxyModule::PlayoutDelay(uint16_t* delayMS) const
{
    return threadInvokeI32([delayMS](const auto& pm) {
        return pm->PlayoutDelay(delayMS); });
}

bool AudioDeviceProxyModule::BuiltInAECIsAvailable() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->BuiltInAECIsAvailable();
    }, false);
}

bool AudioDeviceProxyModule::BuiltInAGCIsAvailable() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->BuiltInAGCIsAvailable();
    }, false);
}

bool AudioDeviceProxyModule::BuiltInNSIsAvailable() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->BuiltInNSIsAvailable();
    }, false);
}

int32_t AudioDeviceProxyModule::EnableBuiltInAEC(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->EnableBuiltInAEC(enable); });
}

int32_t AudioDeviceProxyModule::EnableBuiltInAGC(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->EnableBuiltInAGC(enable); });
}

int32_t AudioDeviceProxyModule::EnableBuiltInNS(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->EnableBuiltInNS(enable); });
}

int32_t AudioDeviceProxyModule::GetPlayoutUnderrunCount() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->GetPlayoutUnderrunCount(); });
}

std::optional<webrtc::AudioDeviceModule::Stats> AudioDeviceProxyModule::GetStats() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->GetStats();
    }, std::optional<Stats>{});
}

void AudioDeviceProxyModule::close()
{
    _listeners.clear();
    webrtc::scoped_refptr<webrtc::AudioDeviceModule> impl;
    {
        LOCK_WRITE_SAFE_OBJ(_impl);
        impl = _impl.take();
    }
    if (impl) {
        invokeInThread(workingThread().get(),  [impl = std::move(impl)]() mutable {
            rtc::scoped_refptr<webrtc::AudioDeviceModule>().swap(impl);
        });
    }
}

void AudioDeviceProxyModule::addListener(AudioDeviceModuleListener* listener)
{
    _listeners.add(listener);
}

void AudioDeviceProxyModule::removeListener(AudioDeviceModuleListener* listener)
{
    _listeners.remove(listener);
}

MediaDevice AudioDeviceProxyModule::defaultRecordingDevice() const
{
    return defaultDevice(true);
}

MediaDevice AudioDeviceProxyModule::defaultPlayoutDevice() const
{
    return defaultDevice(false);
}

bool AudioDeviceProxyModule::setRecordingDevice(const MediaDevice& device)
{
    return threadInvokeR([this, &device](const auto& pm) {
        if (const auto ndx = get(true, device, pm)) {
            return 0 == changeRecordingDevice(ndx.value(), pm);
        }
        return false;
    }, false);
}

bool AudioDeviceProxyModule::setPlayoutDevice(const MediaDevice& device)
{
    return threadInvokeR([this, &device](const auto& pm) {
        if (const auto ndx = get(false, device, pm)) {
            return 0 == changePlayoutDevice(ndx.value(), pm);
        }
        return false;
    }, false);
}

std::vector<MediaDevice> AudioDeviceProxyModule::recordingDevices() const
{
    return enumerate(true);
}

std::vector<MediaDevice> AudioDeviceProxyModule::playoutDevices() const
{
    return enumerate(false);
}

std::string_view AudioDeviceProxyModule::logCategory() const
{
    static const std::string_view category("adm_proxy");
    return category;
}

std::optional<MediaDevice> AudioDeviceProxyModule::
    get(bool recording, uint16_t ndx,
        const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    std::optional<MediaDevice> dev;
    if (adm) {
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([ndx, &dev](uint16_t devNdx,
                                         std::string_view name,
                                         std::string_view guid) {
            if (ndx == devNdx) {
                dev = make(name, guid);
            }
            return dev.has_value();
        });
    }
    return dev;
}

std::optional<MediaDevice> AudioDeviceProxyModule::
    get(bool recording,
        [[maybe_unused]] WindowsDeviceType type,
        [[maybe_unused]] const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    std::optional<MediaDevice> device;
#ifdef WEBRTC_MAC
    if (adm) {
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([&device](uint16_t,
                                       std::string_view name,
                                       std::string_view guid) {
            if (isDefault(name)) {
                device = make(name, guid);
            }
            return device.has_value();
        });
    }
#endif
    return device;
}

std::optional<uint16_t> AudioDeviceProxyModule::
    get(bool recording, const MediaDevice& device,
        const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    std::optional<uint16_t> index;
    if (!device.empty()) {
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([&device, &index](uint16_t ndx,
                                               std::string_view name,
                                               std::string_view guid) {
            if (device._name == name && device._guid == guid) {
                index = ndx;
            }
            return index.has_value();
        });
    }
    return index;
}

std::shared_ptr<rtc::Thread> AudioDeviceProxyModule::workingThread() const
{
    return _listeners.thread().lock();
}

std::vector<MediaDevice> AudioDeviceProxyModule::enumerate(bool recording) const
{
    return threadInvokeR([recording](const auto& pm) {
        std::vector<MediaDevice> devices;
        const AudioDevicesEnumerator enumerator(recording, pm);
        enumerator.enumerate([&devices](std::string_view name, std::string_view guid) {
            devices.push_back(make(name, guid));
        });
        return devices;
    }, std::vector<MediaDevice>{});
}

MediaDevice AudioDeviceProxyModule::defaultDevice(bool recording) const
{
    return threadInvokeR([recording](const auto& pm) {
        static constexpr auto type = WindowsDeviceType::kDefaultCommunicationDevice;
        return get(recording, type, pm).value_or(MediaDevice{});
    }, MediaDevice{});
}

void AudioDeviceProxyModule::requestRecordingAuthorizationStatus() const
{
    MediaAuthorization::query(MediaAuthorizationKind::Microphone, true, logger());
}

void AudioDeviceProxyModule::setRecordingDevice(uint16_t ndx,
                                                const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    if (const auto dev = get(true, ndx, adm)) {
        changeRecordingDevice(dev.value());
    }
}

void AudioDeviceProxyModule::setRecordingDevice(WindowsDeviceType type,
                                                const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    if (const auto dev = get(true, type, adm)) {
        changeRecordingDevice(dev.value());
    }
}

void AudioDeviceProxyModule::setPlayoutDevice(uint16_t ndx,
                                              const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    if (const auto dev = get(false, ndx, adm)) {
        changePlayoutDevice(dev.value());
    }
}

void AudioDeviceProxyModule::setPlayoutDevice(WindowsDeviceType type,
                                              const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    // TODO: implement it
    if (const auto dev = get(false, type, adm)) {
        changePlayoutDevice(dev.value());
    }
}

void AudioDeviceProxyModule::changeRecordingDevice(const MediaDevice& device)
{
    if (!device.empty()) {
        bool changed = false;
        {
            LOCK_WRITE_SAFE_OBJ(_recordingDev);
            if (device != _recordingDev.constRef()) {
                _recordingDev = device;
                changed = true;
            }
        }
        if (changed) {
            _listeners.invoke(&AudioDeviceModuleListener::onRecordingChanged, device);
        }
    }
}

void AudioDeviceProxyModule::changePlayoutDevice(const MediaDevice& device)
{
    if (!device.empty()) {
        bool changed = false;
        {
            LOCK_WRITE_SAFE_OBJ(_playoutDev);
            if (device != _playoutDev.constRef()) {
                _playoutDev = device;
                changed = true;
            }
        }
        if (changed) {
            _listeners.invoke(&AudioDeviceModuleListener::onPlayoutChanged, device);
        }
    }
}

int32_t AudioDeviceProxyModule::changeRecordingDevice(uint16_t index,
                                                      const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    int32_t result = -1;
    if (adm) {
        {
            const ScopedAudioBlocker<true> blocker(*this);
            result = adm->SetRecordingDevice(index);
        }
        if (0 == result) {
            setRecordingDevice(index, adm);
        }
    }
    return result;
}

int32_t AudioDeviceProxyModule::changePlayoutDevice(uint16_t index,
                                                    const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    int32_t result = -1;
    if (adm) {
        {
            const ScopedAudioBlocker<false> blocker(*this);
            result = adm->SetPlayoutDevice(index);
        }
        if (0 == result) {
            setPlayoutDevice(index, adm);
        }
    }
    return result;
}

template <typename Handler>
void AudioDeviceProxyModule::threadInvoke(Handler handler) const
{
    LOCK_READ_SAFE_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        invokeInThread(workingThread().get(), [&impl, handler = std::move(handler)]() {
            handler(impl);
        });
    }
}

template <typename Handler, typename R>
R AudioDeviceProxyModule::threadInvokeR(Handler handler, R defaultVal) const
{
    LOCK_READ_SAFE_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return invokeInThreadR(workingThread().get(), [&impl, handler = std::move(handler)]() {
            return handler(impl);
        }, std::move(defaultVal));
    }
    return defaultVal;
}

template <typename Handler>
int32_t AudioDeviceProxyModule::threadInvokeI32(Handler handler, int32_t defaultVal) const
{
    return threadInvokeR(std::move(handler), defaultVal);
}

template<bool recording>
AudioDeviceProxyModule::ScopedAudioBlocker<recording>::
    ScopedAudioBlocker(const AudioDeviceProxyModule& adm)
    : _impl(adm._impl.constRef())
    , _adm(adm)
{
    if constexpr (recording) {
        if (_impl->Recording()) {
            const auto res = _impl->StopRecording();
            if (0 == res) {
                _needRestart = true;
            }
            else {
                _adm.logWarning("recording stop failed, error code: " + std::to_string(res));
            }
        }
    }
    else {
        if (_impl->Playing()) {
            const auto res = _impl->StopPlayout();
            if (0 == res) {
                _needRestart = true;
            }
            else {
                _adm.logWarning("playout stop failed, error code: " + std::to_string(res));
            }
        }
    }
}

template<bool recording>
AudioDeviceProxyModule::ScopedAudioBlocker<recording>::~ScopedAudioBlocker()
{
    if (_needRestart) {
        if constexpr (recording) {
            auto res = _impl->InitRecording();
            if (0 != res) {
                _adm.logWarning("failed to re-initialize recording, error code: " + std::to_string(res));
            }
            else {
                res = _impl->StartRecording();
                if (0 != res) {
                    _adm.logWarning("failed to restart recording, error code: " + std::to_string(res));
                }
            }
        }
        else {
            auto res = _impl->InitPlayout();
            if (0 != res) {
                _adm.logWarning("failed to re-initialize playout, error code: " + std::to_string(res));
            }
            else {
                res = _impl->StartPlayout();
                if (0 != res) {
                    _adm.logWarning("failed to restart playout, error code: " + std::to_string(res));
                }
            }
        }
    }
}

} // namespace LiveKitCpp
