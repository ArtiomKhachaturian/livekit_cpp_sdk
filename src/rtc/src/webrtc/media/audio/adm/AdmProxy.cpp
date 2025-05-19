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
#include "AdmScopedBlocker.h"
#include "AdmProxyTransport.h"
#include "MediaAuthorization.h"
#include "Logger.h"
#include "Utils.h"
#include "ThreadUtils.h"
#include <api/make_ref_counted.h>
#include <rtc_base/thread.h>
#ifdef WEBRTC_WIN
#include <modules/audio_device/win/core_audio_utility_win.h>
#include <atlbase.h>     //CComPtr support
#include <Functiondiscoverykeys_devpkey.h>
#include <Mmdeviceapi.h> // MMDevice
#endif

namespace
{

using namespace LiveKitCpp;

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

// TODO: expose this class for public
class RtcLogger : public Bricks::Logger
{
public:
    RtcLogger() = default;
    bool canLog(Bricks::LoggingSeverity severity) const final;
    void log(Bricks::LoggingSeverity severity,
             std::string_view message,
             std::string_view category) final;
private:
    static std::optional<rtc::LoggingSeverity> map(Bricks::LoggingSeverity sev);
};

} // namespace

namespace LiveKitCpp
{

AdmProxy::AdmProxy(const std::shared_ptr<webrtc::Thread>& workingThread,
                   const std::shared_ptr<webrtc::TaskQueueBase>& signalingQueue,
                   rtc::scoped_refptr<webrtc::AudioDeviceModule> impl)
    : _workingThread(workingThread)
    , _transport(std::make_unique<AdmProxyTransport>())
    , _impl(std::move(impl))
    , _recState(true, signalingQueue)
    , _playState(false, signalingQueue)
{
    if (RTC_LOG_CHECK_LEVEL(LS_INFO)) {
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
            RTC_LOG(LS_INFO) << "accessibility of built-in audio processing: " << join(info, ", ");
        }
    }
}

AdmProxy::~AdmProxy()
{
    close();
}

rtc::scoped_refptr<AdmProxy> AdmProxy::
    create(const std::shared_ptr<webrtc::Thread>& workingThread,
           const std::shared_ptr<webrtc::TaskQueueBase>& signalingQueue,
           webrtc::TaskQueueFactory* taskQueueFactory)
{
    if (workingThread && signalingQueue) {
        auto impl = invokeInThreadR(workingThread.get(), [taskQueueFactory](){
            return defaultAdm(taskQueueFactory);
        });
        if (impl) {
            return rtc::make_ref_counted<AdmProxy>(workingThread,
                                                   signalingQueue,
                                                   std::move(impl));
        }
    }
    return {};
}

int32_t AdmProxy::ActiveAudioLayer(AudioLayer* audioLayer) const
{
    if (audioLayer) {
        return threadInvokeI32([audioLayer](const auto& pm) {
            return pm->ActiveAudioLayer(audioLayer); });
    }
    return -1;
}

// Full-duplex transportation of PCM audio
int32_t AdmProxy::RegisterAudioCallback(webrtc::AudioTransport* audioCallback)
{
    return threadInvokeI32([this, audioCallback](const auto& pm) {
        int32_t res = -1;
        LOCK_READ_SAFE_OBJ(_transport);
        if (!audioCallback) {
            res = pm->RegisterAudioCallback(nullptr);
            if (const auto& transport = _transport.constRef()) {
                transport->setTargetTransport(nullptr);
            }
        }
        else {
            if (const auto& transport = _transport.constRef()) {
                transport->setTargetTransport(audioCallback);
                res = pm->RegisterAudioCallback(transport.get());
            }
        }
        return res;
    });
}

// Main initialization and termination
int32_t AdmProxy::AdmProxy::Init()
{
    return threadInvokeI32([this](const auto& pm) {
        return pm->Init();
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
            const AdmScopedBlocker<false> blocker(pm);
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
            const AdmScopedBlocker<true> blocker(pm);
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
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->PlayoutIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::InitPlayout()
{
    return threadInvokeI32([this](const auto& pm) {
        const auto res = pm->InitPlayout();
        if (0 == res) {
            _playState.update(pm);
        }
        return res;
    });
}

bool AdmProxy::PlayoutIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->PlayoutIsInitialized(); });
}

int32_t AdmProxy::RecordingIsAvailable(bool* available)
{
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->RecordingIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::InitRecording()
{
    return threadInvokeI32([this](const auto& pm) {
        const auto res = pm->InitRecording();
        if (0 == res) {
            _recState.update(pm);
        }
        return res;
    });
}

bool AdmProxy::RecordingIsInitialized() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->RecordingIsInitialized(); });
}

// Audio transport control
int32_t AdmProxy::StartPlayout()
{
    const auto res = threadInvokeI32([](const auto& pm) {
        return pm->StartPlayout(); });
    if (0 == res) {
        _playState.setStarted();
    }
    return res;
}

int32_t AdmProxy::StopPlayout()
{
    const auto res = threadInvokeI32([](const auto& pm) {
        return pm->StopPlayout(); });
    if (0 == res) {
        _playState.setStopped();
    }
    return res;
}

bool AdmProxy::Playing() const
{
    return threadInvokeI32([](const auto& pm) {
        return pm->Playing(); });
}

int32_t AdmProxy::StartRecording()
{
    const auto res = threadInvokeI32([this](const auto& pm) {
        requestRecordingAuthorizationStatus();
#ifdef WEBRTC_WIN
        if (_builtInAecEnabled) {
            auto res = pm->InitPlayout();
            if (0 != res) {
                return res;
            }
            res = pm->StartPlayout();
            if (0 != res) {
                return res;
            }
        }
#endif
        return pm->StartRecording();
    });
    if (0 == res) {
        _recState.setStarted();
    }
    return res;
}

int32_t AdmProxy::StopRecording()
{
    const auto res = threadInvokeI32([](const auto& pm) {
        return pm->StopRecording();
    });
    if (0 == res) {
        _recState.setStopped();
    }
    return res;
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
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->SpeakerVolumeIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::SetSpeakerVolume(uint32_t volume)
{
    const auto res = threadInvokeI32([volume](const auto& pm) {
        return pm->SetSpeakerVolume(volume); });
    if (0 == res) {
        _playState.setVolume(volume);
    }
    return res;
}

int32_t AdmProxy::SpeakerVolume(uint32_t* volume) const
{
    if (volume) {
        return threadInvokeI32([volume](const auto& pm) {
            return pm->SpeakerVolume(volume); });
    }
    return -1;
}

int32_t AdmProxy::MaxSpeakerVolume(uint32_t* maxVolume) const
{
    if (maxVolume) {
        return threadInvokeI32([maxVolume](const auto& pm) {
            return pm->MaxSpeakerVolume(maxVolume); });
    }
    return -1;
}

int32_t AdmProxy::MinSpeakerVolume(uint32_t* minVolume) const
{
    if (minVolume) {
        return threadInvokeI32([minVolume](const auto& pm) {
            return pm->MinSpeakerVolume(minVolume); });
    }
    return -1;
}

// Microphone volume controls
int32_t AdmProxy::MicrophoneVolumeIsAvailable(bool* available)
{
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->MicrophoneVolumeIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::SetMicrophoneVolume(uint32_t volume)
{
    const auto res = threadInvokeI32([volume](const auto& pm) {
        return pm->SetMicrophoneVolume(volume); });
    if (0 == res) {
        _recState.setVolume(volume);
    }
    return res;
}

int32_t AdmProxy::MicrophoneVolume(uint32_t* volume) const
{
    if (volume) {
        return threadInvokeI32([volume](const auto& pm) {
            return pm->MicrophoneVolume(volume); });
    }
    return -1;
}

int32_t AdmProxy::MaxMicrophoneVolume(uint32_t* maxVolume) const
{
    if (maxVolume) {
        return threadInvokeI32([maxVolume](const auto& pm) {
            return pm->MaxMicrophoneVolume(maxVolume); });
    }
    return -1;
}

int32_t AdmProxy::MinMicrophoneVolume(uint32_t* minVolume) const
{
    if (minVolume) {
        return threadInvokeI32([minVolume](const auto& pm) {
            return pm->MinMicrophoneVolume(minVolume); });
    }
    return -1;
}

// Speaker mute control
int32_t AdmProxy::SpeakerMuteIsAvailable(bool* available)
{
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->SpeakerMuteIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::SetSpeakerMute(bool mute)
{
    const auto res = threadInvokeI32([mute](const auto& pm) {
        return pm->SetSpeakerMute(mute); });
    if (0 == res) {
        _playState.setMute(mute);
    }
    return res;
}

int32_t AdmProxy::SpeakerMute(bool* muted) const
{
    if (muted) {
        return threadInvokeI32([muted](const auto& pm) {
            return pm->SpeakerMute(muted); });
    }
    return -1;
}

// Microphone mute control
int32_t AdmProxy::MicrophoneMuteIsAvailable(bool* available)
{
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->MicrophoneMuteIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::SetMicrophoneMute(bool mute)
{
    const auto res = threadInvokeI32([mute](const auto& pm) {
        return pm->SetMicrophoneMute(mute); });
    if (0 == res) {
        _recState.setMute(mute);
    }
    return res;
}

int32_t AdmProxy::MicrophoneMute(bool* muted) const
{
    if (muted) {
        return threadInvokeI32([muted](const auto& pm) {
            return pm->MicrophoneMute(muted); });
    }
    return -1;
}

// Stereo support
int32_t AdmProxy::StereoPlayoutIsAvailable(bool* available) const
{
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->StereoPlayoutIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::SetStereoPlayout(bool enable)
{
    const auto res = threadInvokeI32([enable](const auto& pm) {
        return pm->SetStereoPlayout(enable); });
    if (0 == res) {
        _playState.setStereo(enable);
    }
    return res;
}

int32_t AdmProxy::StereoPlayout(bool* enabled) const
{
    if (enabled) {
        return threadInvokeI32([enabled](const auto& pm) {
            return pm->StereoPlayout(enabled); });
    }
    return -1;
}

int32_t AdmProxy::StereoRecordingIsAvailable(bool* available) const
{
    if (available) {
        return threadInvokeI32([available](const auto& pm) {
            return pm->StereoRecordingIsAvailable(available); });
    }
    return -1;
}

int32_t AdmProxy::SetStereoRecording(bool enable)
{
    const auto res = threadInvokeI32([enable](const auto& pm) {
        return pm->SetStereoRecording(enable); });
    if (0 == res) {
        _recState.setStereo(enable);
    }
    return res;
}

int32_t AdmProxy::StereoRecording(bool* enabled) const
{
    if (enabled) {
        return threadInvokeI32([enabled](const auto& pm) {
            return pm->StereoRecording(enabled); });
    }
    return -1;
}

int32_t AdmProxy::PlayoutDelay(uint16_t* delayMS) const
{
    if (delayMS) {
        return threadInvokeI32([delayMS](const auto& pm) {
            return pm->PlayoutDelay(delayMS); });
    }
    return -1;
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
    return threadInvokeI32([this, enable](const auto& pm) {
        const auto res = pm->EnableBuiltInAEC(enable);
#ifdef WEBRTC_WIN
        if (0 == res) {
            _builtInAecEnabled = enable;
        }
#endif
        return res;
    });
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
    _recState.clearListeners();
    _playState.clearListeners();
    LOCK_WRITE_SAFE_OBJ(_impl);
    if (auto impl = _impl.take()) {
        LOCK_WRITE_SAFE_OBJ(_transport);
        auto transport = _transport.take();
        if (transport) {
            transport->close();
        }
        postTask(workingThread(), [impl = std::move(impl), transport = std::move(transport)]() mutable {
            impl->RegisterAudioCallback(nullptr);
            AdmPtr().swap(impl);
            transport.reset();
        });
    }
}

void AdmProxy::registerRecordingSink(webrtc::AudioTrackSinkInterface* sink, bool reg)
{
    LOCK_READ_SAFE_OBJ(_transport);
    if (const auto& transport = _transport.constRef()) {
        if (reg) {
            transport->addSink(sink);
        }
        else {
            transport->removeSink(sink);
        }
    }
}

void AdmProxy::registerRecordingListener(AdmProxyListener* l, bool reg)
{
    _recState.registereListener(l, reg);
}

void AdmProxy::registerPlayoutListener(AdmProxyListener* l, bool reg)
{
    _playState.registereListener(l, reg);
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

std::optional<MediaDeviceInfo> AdmProxy::get(bool recording, uint16_t ndx,
                                             const AdmPtr& adm)
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

std::optional<MediaDeviceInfo> AdmProxy::
    get(bool recording, [[maybe_unused]] WindowsDeviceType type,
       [[maybe_unused]] const AdmPtr& adm)
{
#ifdef WEBRTC_MAC
    if (adm) {
        std::optional<MediaDeviceInfo> device;
        const AudioDevicesEnumerator enumerator(recording, adm);
        enumerator.enumerate([&device](uint16_t,
                                       std::string_view name,
                                       std::string_view guid) {
            if (isDefault(name)) {
                device = make(name, guid);
            }
            return !device.has_value();
        });
        return device;
    }
#elif defined (WEBRTC_WIN)
    if (initializeComForThisThread()) {
        const EDataFlow flow = recording ? EDataFlow::eCapture : EDataFlow::eRender;
        // https://webrtc.googlesource.com/src/+/refs/heads/main/modules/audio_device/win/audio_device_core_win.cc#1452
        ERole role = ERole::eCommunications;
        if (AudioDeviceModule::kDefaultDevice == type) {
            role = ERole::eConsole;
        }
        CComPtr<IMMDeviceEnumerator> enumerator;
        // get enumerator for audio endpoint devices
        HRESULT hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL);
        if (SUCCEEDED(hr)) {
            CComPtr<IMMDevice> dev;
            hr = enumerator->GetDefaultAudioEndpoint(flow, role, &dev);
            if (SUCCEEDED(hr)) {
                MediaDeviceInfo device;
                LPWSTR pwstrDeviceId = NULL;
                if (SUCCEEDED(dev->GetId(&pwstrDeviceId))) {
                    device._guid = fromWideChar(pwstrDeviceId);
                    ::CoTaskMemFree(pwstrDeviceId);
                }
                if (!device._guid.empty()) {
                    CComPtr<IPropertyStore> props;
                    if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &props))) {
                        // get the endpoint device's friendly-name property
                        // https://stackoverflow.com/questions/41097521/how-to-get-whether-a-speaker-plugged-or-unplugged
                        using namespace webrtc::webrtc_win;
                        ScopedPropVariant name;
                        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, name.Receive()))) {
                            switch (name.get().vt) {
                                case VT_LPWSTR:
                                    device._name = fromWideChar(name.get().pwszVal);
                                    break;
                                case VT_LPSTR:
                                    device._name = std::string(name.get().pszVal);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                if (!device.empty()) {
                    return device;
                }
            }
        }
    }    
#endif
    return {};
}

std::optional<uint16_t> AdmProxy::get(bool recording, const MediaDeviceInfo& info,
                                      const AdmPtr& adm)
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

AdmPtr AdmProxy::defaultAdm(webrtc::TaskQueueFactory* taskQueueFactory)
{
    return webrtc::AudioDeviceModule::Create(AudioLayer::kPlatformDefaultAudio, taskQueueFactory);
}

std::shared_ptr<webrtc::Thread> AdmProxy::workingThread() const
{
    return _workingThread.lock();
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
    MediaAuthorization::query(MediaAuthorizationKind::Microphone, true,
                              std::make_shared<RtcLogger>());
}

void AdmProxy::setRecordingDevice(uint16_t ndx, const AdmPtr& adm)
{
    if (const auto dev = get(true, ndx, adm)) {
        changeRecordingDevice(dev.value(), adm);
    }
}

void AdmProxy::setRecordingDevice(WindowsDeviceType type, const AdmPtr& adm)
{
    if (const auto dev = get(true, type, adm)) {
        changeRecordingDevice(dev.value(), adm);
    }
}

void AdmProxy::setPlayoutDevice(uint16_t ndx, const AdmPtr& adm)
{
    if (const auto dev = get(false, ndx, adm)) {
        changePlayoutDevice(dev.value(), adm);
    }
}

void AdmProxy::setPlayoutDevice(WindowsDeviceType type, const AdmPtr& adm)
{
    // TODO: implement it
    if (const auto dev = get(false, type, adm)) {
        changePlayoutDevice(dev.value(), adm);
    }
}

void AdmProxy::changeRecordingDevice(const MediaDeviceInfo& info, const AdmPtr& adm)
{
    if (!info.empty()) {
        _recState.setCurrentDevice(info, adm);
    }
}

void AdmProxy::changePlayoutDevice(const MediaDeviceInfo& info, const AdmPtr& adm)
{
    if (!info.empty()) {
        _playState.setCurrentDevice(info, adm);
    }
}

int32_t AdmProxy::changeRecordingDevice(uint16_t index, const AdmPtr& adm)
{
    int32_t result = -1;
    if (adm) {
        {
            const AdmScopedBlocker<true> blocker(adm);
            result = adm->SetRecordingDevice(index);
        }
        if (0 == result) {
            setRecordingDevice(index, adm);
        }
    }
    return result;
}

int32_t AdmProxy::changePlayoutDevice(uint16_t index, const AdmPtr& adm)
{
    int32_t result = -1;
    if (adm) {
        {
            const AdmScopedBlocker<false> blocker(adm);
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

} // namespace LiveKitCpp

namespace
{

bool RtcLogger::canLog(Bricks::LoggingSeverity severity) const
{
    if (const auto sev = map(severity)) {
        return RTC_LOG_CHECK_LEVEL_V(sev.value());
    }
    return false;
}

void RtcLogger::log(Bricks::LoggingSeverity severity,
                    std::string_view message,
                    std::string_view category)
{
    if (!message.empty()) {
        if (const auto sev = map(severity)) {
            if (!category.empty()) {
                RTC_LOG_V(sev.value()) << category << "/" << message;
            }
            else {
                RTC_LOG_V(sev.value()) << message;
            }
        }
    }
}

std::optional<rtc::LoggingSeverity> RtcLogger::map(Bricks::LoggingSeverity sev)
{
    switch (sev) {
        case Bricks::LoggingSeverity::Verbose:
            return rtc::LS_VERBOSE;
        case Bricks::LoggingSeverity::Info:
            return rtc::LS_INFO;
        case Bricks::LoggingSeverity::Warning:
            return rtc::LS_WARNING;
        case Bricks::LoggingSeverity::Error:
            return rtc::LS_ERROR;
        default:
            break;
    }
    return std::nullopt;
}

}
