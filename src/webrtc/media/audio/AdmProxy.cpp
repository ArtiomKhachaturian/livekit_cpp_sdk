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
#include "AdmProxy.h"
#include "AudioDevicesEnumerator.h"
#include "AdmProxyListener.h"
#include "MediaAuthorization.h"
#include "Utils.h"
#include "ThreadUtils.h"
#include <api/make_ref_counted.h>
#include <rtc_base/thread.h>

namespace
{

using AdmPtr = webrtc::scoped_refptr<webrtc::AudioDeviceModule>;
using namespace LiveKitCpp;

inline AdmPtr defaultAdm(webrtc::TaskQueueFactory* taskQueueFactory) {
    return webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::kPlatformDefaultAudio,
                                             taskQueueFactory);
}

inline MediaDeviceInfo make(std::string_view name, std::string_view guid) {
    MediaDeviceInfo info;
    info._name = name;
    info._guid = guid;
    return info;
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
class AdmProxy::ScopedAudioBlocker
{
public:
    ScopedAudioBlocker(const AdmProxy& adm);
    ~ScopedAudioBlocker();
private:
    const AdmPtr& _impl;
    const AdmProxy& _adm;
    bool _needRestart = false;
};

AdmProxy::AdmProxy(const std::shared_ptr<rtc::Thread>& thread,
                   rtc::scoped_refptr<webrtc::AudioDeviceModule> impl,
                   const std::shared_ptr<Bricks::Logger>& logger)
    :  Bricks::LoggableS<webrtc::AudioDeviceModule>(logger)
    , _thread(thread)
    , _impl(std::move(impl))
    , _listeners(thread)
    , _weakFactory(this)
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

AdmProxy::~AdmProxy()
{
    close();
}

rtc::scoped_refptr<AdmProxy> AdmProxy::
    create(const std::shared_ptr<rtc::Thread>& thread,
           webrtc::TaskQueueFactory* taskQueueFactory,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (thread) {
        auto impl = invokeInThreadR(thread.get(), [taskQueueFactory](){
            return defaultAdm(taskQueueFactory);
        });
        if (impl) {
            return rtc::make_ref_counted<AdmProxy>(thread, std::move(impl), logger);
        }
    }
    return nullptr;
}

int32_t AdmProxy::ActiveAudioLayer(AudioLayer* audioLayer) const
{
    return threadInvokeI32([audioLayer](const auto& pm) {
        return pm->ActiveAudioLayer(audioLayer); });
}

// Full-duplex transportation of PCM audio
int32_t AdmProxy::RegisterAudioCallback(webrtc::AudioTransport* audioCallback)
{
    return threadInvokeI32([audioCallback](const auto& pm) {
        return pm->RegisterAudioCallback(audioCallback); });
}

// Main initialization and termination
int32_t AdmProxy::AdmProxy::Init()
{
    return threadInvokeI32([this](const auto& pm) {
        const auto res = pm->Init();
        if (0 == res) {
            bool enabled = false;
            if (0 == pm->StereoRecording(&enabled)) {
                _stereoRecording = enabled;
            }
            if (0 == pm->StereoPlayout(&enabled)) {
                _stereoPlayout = enabled;
            }
        }
        return res;
    });
}

int32_t AdmProxy::AdmProxy::Terminate()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Terminate(); });
}

bool AdmProxy::Initialized() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->Initialized();
    }, false);
}

// Device enumeration
int16_t AdmProxy::PlayoutDevices()
{
    return threadInvokeR([](const auto& pm) {
        return pm->PlayoutDevices();
    }, int16_t(0));
}

int16_t AdmProxy::RecordingDevices()
{
    return threadInvokeR([](const auto& pm) {
        return pm->RecordingDevices();
    }, int16_t(0));
}

int32_t AdmProxy::PlayoutDeviceName(uint16_t index,
                                    char name[webrtc::kAdmMaxDeviceNameSize],
                                    char guid[webrtc::kAdmMaxGuidSize])
{
    return threadInvokeI32([index, &name, &guid](const auto& pm) {
        return pm->PlayoutDeviceName(index, name, guid); });
}

int32_t AdmProxy::RecordingDeviceName(uint16_t index,
                                      char name[webrtc::kAdmMaxDeviceNameSize],
                                      char guid[webrtc::kAdmMaxGuidSize])
{
    return threadInvokeI32([index, &name, &guid](const auto& pm) {
        return pm->RecordingDeviceName(index, name, guid); });
}

// Device selection
int32_t AdmProxy::SetPlayoutDevice(uint16_t index)
{
    return threadInvokeI32([this, index](const auto& pm) {
        return changePlayoutDevice(index, pm);
    });
}

int32_t AdmProxy::SetPlayoutDevice(WindowsDeviceType device)
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

int32_t AdmProxy::SetRecordingDevice(uint16_t index)
{
    return threadInvokeI32([this, index](const auto& pm) {
        return changeRecordingDevice(index, pm);
    });
}

int32_t AdmProxy::SetRecordingDevice(WindowsDeviceType device)
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
int32_t AdmProxy::PlayoutIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->PlayoutIsAvailable(available); });
}

int32_t AdmProxy::InitPlayout()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitPlayout(); });
}

bool AdmProxy::PlayoutIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->PlayoutIsInitialized(); });
}

int32_t AdmProxy::RecordingIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->RecordingIsAvailable(available); });
}

int32_t AdmProxy::InitRecording()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitRecording(); });
}

bool AdmProxy::RecordingIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->RecordingIsInitialized(); });
}

// Audio transport control
int32_t AdmProxy::StartPlayout()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->StartPlayout(); });
}

int32_t AdmProxy::StopPlayout()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->StopPlayout(); });
}

bool AdmProxy::Playing() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Playing(); });
}

int32_t AdmProxy::StartRecording()
{
    return threadInvokeI32([this](const auto& pm) {
        requestRecordingAuthorizationStatus();
        return pm->StartRecording(); });
}

int32_t AdmProxy::StopRecording()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->StopRecording(); });
}

bool AdmProxy::Recording() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Recording(); });
}

// Audio mixer initialization
int32_t AdmProxy::InitSpeaker()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitSpeaker(); });
}

bool AdmProxy::SpeakerIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->SpeakerIsInitialized(); });
}

int32_t AdmProxy::InitMicrophone()
{
    return threadInvokeI32([](const auto& pm) {
        return pm->InitMicrophone(); });
}

bool AdmProxy::MicrophoneIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->MicrophoneIsInitialized(); });
}

// Speaker volume controls
int32_t AdmProxy::SpeakerVolumeIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->SpeakerVolumeIsAvailable(available); });
}

int32_t AdmProxy::SetSpeakerVolume(uint32_t volume)
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->SetSpeakerVolume(volume); });
}

int32_t AdmProxy::SpeakerVolume(uint32_t* volume) const
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->SpeakerVolume(volume); });
}

int32_t AdmProxy::MaxSpeakerVolume(uint32_t* maxVolume) const
{
    return threadInvokeI32([maxVolume](const auto& pm) {
        return pm->MaxSpeakerVolume(maxVolume); });
}

int32_t AdmProxy::MinSpeakerVolume(uint32_t* minVolume) const
{
    return threadInvokeI32([minVolume](const auto& pm) {
        return pm->MinSpeakerVolume(minVolume); });
}

// Microphone volume controls
int32_t AdmProxy::MicrophoneVolumeIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->MicrophoneVolumeIsAvailable(available); });
}

int32_t AdmProxy::SetMicrophoneVolume(uint32_t volume)
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->SetMicrophoneVolume(volume); });
}

int32_t AdmProxy::MicrophoneVolume(uint32_t* volume) const
{
    return threadInvokeI32([volume](const auto& pm) {
        return pm->MicrophoneVolume(volume); });
}

int32_t AdmProxy::MaxMicrophoneVolume(uint32_t* maxVolume) const
{
    return threadInvokeI32([maxVolume](const auto& pm) {
        return pm->MaxMicrophoneVolume(maxVolume); });
}

int32_t AdmProxy::MinMicrophoneVolume(uint32_t* minVolume) const
{
    return threadInvokeI32([minVolume](const auto& pm) {
        return pm->MinMicrophoneVolume(minVolume); });
}

// Speaker mute control
int32_t AdmProxy::SpeakerMuteIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->SpeakerMuteIsAvailable(available); });
}

int32_t AdmProxy::SetSpeakerMute(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->SetSpeakerMute(enable); });
}

int32_t AdmProxy::SpeakerMute(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->SpeakerMute(enabled); });
}

// Microphone mute control
int32_t AdmProxy::MicrophoneMuteIsAvailable(bool* available)
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->MicrophoneMuteIsAvailable(available); });
}

int32_t AdmProxy::SetMicrophoneMute(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->SetMicrophoneMute(enable); });
}

int32_t AdmProxy::MicrophoneMute(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->MicrophoneMute(enabled); });
}

// Stereo support
int32_t AdmProxy::StereoPlayoutIsAvailable(bool* available) const
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->StereoPlayoutIsAvailable(available); });
}

int32_t AdmProxy::SetStereoPlayout(bool enable)
{
    const auto res = threadInvokeI32([enable](const auto& pm) {
        return pm->SetStereoPlayout(enable); });
    if (0 == res) {
        _stereoPlayout = enable;
    }
    return res;
}

int32_t AdmProxy::StereoPlayout(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->StereoPlayout(enabled); });
}

int32_t AdmProxy::StereoRecordingIsAvailable(bool* available) const
{
    return threadInvokeI32([available](const auto& pm) {
        return pm->StereoRecordingIsAvailable(available); });
}

int32_t AdmProxy::SetStereoRecording(bool enable)
{
    const auto res = threadInvokeI32([enable](const auto& pm) {
        return pm->SetStereoRecording(enable); });
    if (0 == res) {
        _stereoRecording = enable;
    }
    return res;
}

int32_t AdmProxy::StereoRecording(bool* enabled) const
{
    return threadInvokeI32([enabled](const auto& pm) {
        return pm->StereoRecording(enabled); });
}

int32_t AdmProxy::PlayoutDelay(uint16_t* delayMS) const
{
    return threadInvokeI32([delayMS](const auto& pm) {
        return pm->PlayoutDelay(delayMS); });
}

bool AdmProxy::BuiltInAECIsAvailable() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->BuiltInAECIsAvailable();
    }, false);
}

bool AdmProxy::BuiltInAGCIsAvailable() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->BuiltInAGCIsAvailable();
    }, false);
}

bool AdmProxy::BuiltInNSIsAvailable() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->BuiltInNSIsAvailable();
    }, false);
}

int32_t AdmProxy::EnableBuiltInAEC(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->EnableBuiltInAEC(enable); });
}

int32_t AdmProxy::EnableBuiltInAGC(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->EnableBuiltInAGC(enable); });
}

int32_t AdmProxy::EnableBuiltInNS(bool enable)
{
    return threadInvokeI32([enable](const auto& pm) {
        return pm->EnableBuiltInNS(enable); });
}

int32_t AdmProxy::GetPlayoutUnderrunCount() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->GetPlayoutUnderrunCount(); });
}

std::optional<webrtc::AudioDeviceModule::Stats> AdmProxy::GetStats() const
{
    return threadInvokeR([](const auto& pm) {
        return pm->GetStats();
    }, std::optional<Stats>{});
}

void AdmProxy::close()
{
    _listeners.clear();
    AdmPtr impl;
    {
        LOCK_WRITE_SAFE_OBJ(_impl);
        impl = _impl.take();
    }
    if (impl) {
        postTask(workingThread(), [impl = std::move(impl)]() mutable {
            AdmPtr().swap(impl);
        });
    }
}

void AdmProxy::registerListener(AdmProxyListener* listener, bool reg)
{
    if (reg) {
        _listeners.add(listener);
    }
    else {
        _listeners.remove(listener);
    }
}

MediaDeviceInfo AdmProxy::defaultRecordingDevice() const
{
    return defaultDevice(true);
}

MediaDeviceInfo AdmProxy::defaultPlayoutDevice() const
{
    return defaultDevice(false);
}

bool AdmProxy::setRecordingDevice(const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        return threadInvokeR([this, &info](const auto& pm) {
            if (const auto ndx = get(true, info, pm)) {
                return 0 == changeRecordingDevice(ndx.value(), pm);
            }
            return false;
        }, false);
    }
    return false;
}

bool AdmProxy::setPlayoutDevice(const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        return threadInvokeR([this, &info](const auto& pm) {
            if (const auto ndx = get(false, info, pm)) {
                return 0 == changePlayoutDevice(ndx.value(), pm);
            }
            return false;
        }, false);
    }
    return false;
}

std::vector<MediaDeviceInfo> AdmProxy::recordingDevices() const
{
    return enumerate(true);
}

std::vector<MediaDeviceInfo> AdmProxy::playoutDevices() const
{
    return enumerate(false);
}

std::string_view AdmProxy::logCategory() const
{
    static const std::string_view category("adm_proxy");
    return category;
}

std::optional<MediaDeviceInfo> AdmProxy::get(bool recording, uint16_t ndx,
                                             const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    std::optional<MediaDeviceInfo> dev;
    if (adm) {
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([ndx, &dev](uint16_t devNdx,
                                         std::string_view name,
                                         std::string_view guid) {
            if (ndx == devNdx) {
                dev = make(name, guid);
            }
            return !dev.has_value();
        });
    }
    return dev;
}

std::optional<MediaDeviceInfo> AdmProxy::get(bool recording,
                                             [[maybe_unused]] WindowsDeviceType type,
                                             [[maybe_unused]] const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    std::optional<MediaDeviceInfo> device;
#ifdef WEBRTC_MAC
    if (adm) {
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([&device](uint16_t,
                                       std::string_view name,
                                       std::string_view guid) {
            if (isDefault(name)) {
                device = make(name, guid);
            }
            return !device.has_value();
        });
    }
#endif
    return device;
}

std::optional<uint16_t> AdmProxy::get(bool recording, const MediaDeviceInfo& info,
                                      const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    std::optional<uint16_t> index;
    if (!info.empty()) {
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([&info, &index](uint16_t ndx,
                                             std::string_view name,
                                             std::string_view guid) {
            if (info._name == name && info._guid == guid) {
                index = ndx;
            }
            return !index.has_value();
        });
    }
    return index;
}

std::shared_ptr<rtc::Thread> AdmProxy::workingThread() const
{
    return _thread.lock();
}

std::vector<MediaDeviceInfo> AdmProxy::enumerate(bool recording) const
{
    return threadInvokeR([recording](const auto& pm) {
        std::vector<MediaDeviceInfo> devices;
        const AudioDevicesEnumerator enumerator(recording, pm);
        enumerator.enumerate([&devices](std::string_view name, std::string_view guid) {
            devices.push_back(make(name, guid));
        });
        return devices;
    }, std::vector<MediaDeviceInfo>{});
}

MediaDeviceInfo AdmProxy::defaultDevice(bool recording) const
{
    return threadInvokeR([recording](const auto& pm) {
        static constexpr auto type = WindowsDeviceType::kDefaultCommunicationDevice;
        return get(recording, type, pm).value_or(MediaDeviceInfo{});
    }, MediaDeviceInfo{});
}

void AdmProxy::requestRecordingAuthorizationStatus() const
{
    MediaAuthorization::query(MediaAuthorizationKind::Microphone, true, logger());
}

void AdmProxy::setRecordingDevice(uint16_t ndx,
                                  const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    if (const auto dev = get(true, ndx, adm)) {
        changeRecordingDevice(dev.value());
    }
}

void AdmProxy::setRecordingDevice(WindowsDeviceType type,
                                  const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    if (const auto dev = get(true, type, adm)) {
        changeRecordingDevice(dev.value());
    }
}

void AdmProxy::setPlayoutDevice(uint16_t ndx,
                                const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    if (const auto dev = get(false, ndx, adm)) {
        changePlayoutDevice(dev.value());
    }
}

void AdmProxy::setPlayoutDevice(WindowsDeviceType type,
                                const webrtc::scoped_refptr<webrtc::AudioDeviceModule>& adm)
{
    // TODO: implement it
    if (const auto dev = get(false, type, adm)) {
        changePlayoutDevice(dev.value());
    }
}

void AdmProxy::changeRecordingDevice(const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        bool changed = false;
        {
            LOCK_WRITE_SAFE_OBJ(_recordingDev);
            if (info != _recordingDev.constRef()) {
                _recordingDev = info;
                changed = true;
            }
        }
        if (changed) {
            _listeners.invoke(&AdmProxyListener::onRecordingChanged, info);
        }
    }
}

void AdmProxy::changePlayoutDevice(const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        bool changed = false;
        {
            LOCK_WRITE_SAFE_OBJ(_playoutDev);
            if (info != _playoutDev.constRef()) {
                _playoutDev = info;
                changed = true;
            }
        }
        if (changed) {
            _listeners.invoke(&AdmProxyListener::onPlayoutChanged, info);
        }
    }
}

int32_t AdmProxy::changeRecordingDevice(uint16_t index,
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

int32_t AdmProxy::changePlayoutDevice(uint16_t index,
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
void AdmProxy::threadInvoke(Handler handler) const
{
    LOCK_READ_SAFE_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        invokeInThread(workingThread().get(), [&impl, handler = std::move(handler)]() {
            handler(impl);
        });
    }
}

template <typename Handler, typename R>
R AdmProxy::threadInvokeR(Handler handler, R defaultVal) const
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
int32_t AdmProxy::threadInvokeI32(Handler handler, int32_t defaultVal) const
{
    return threadInvokeR(std::move(handler), defaultVal);
}

template<bool recording>
AdmProxy::ScopedAudioBlocker<recording>::ScopedAudioBlocker(const AdmProxy& adm)
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
AdmProxy::ScopedAudioBlocker<recording>::~ScopedAudioBlocker()
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
