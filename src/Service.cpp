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
#include "Service.h"
#include "Logger.h"
#include "MediaAuthorization.h"
#include "WebsocketEndPoint.h"
#include "WebsocketFactory.h"
#include "NetworkType.h"
#include "VideoOptions.h"
#include "Utils.h"
#include "rtc/ClientInfo.h"
#include "e2e/KeyProviderOptions.h"
#include "e2e/KeyProvider.h"
#ifdef WEBRTC_AVAILABLE
#include "AdmProxyListener.h"
#include "CameraManager.h"
#include "DefaultKeyProvider.h"
#include "Loggable.h"
#include "PeerConnectionFactory.h"
#include "RtcInitializer.h"
#ifdef __APPLE__
#include "AppEnvironment.h"
#elif defined(_WIN32)
#include "WSAInitializer.h"
#endif
#endif

namespace {

#ifdef WEBRTC_AVAILABLE
const std::string_view g_logCategory("service");
#endif

}

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE
class Service::Impl : public Bricks::LoggableS<AdmProxyListener>
{
public:
    Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
         const MicrophoneOptions& microphoneOptions,
         const std::shared_ptr<Bricks::Logger>& logger,
         bool logWebrtcEvents);
    ~Impl();
    const auto& websocketsFactory() const noexcept { return _websocketsFactory; }
    const auto& peerConnectionFactory() const noexcept { return _pcf; }
    MediaDeviceInfo defaultRecordingAudioDevice() const;
    MediaDeviceInfo defaultPlayoutAudioDevice() const;
    bool setRecordingAudioDevice(const MediaDeviceInfo& info);
    MediaDeviceInfo recordingAudioDevice() const;
    bool setPlayoutAudioDevice(const MediaDeviceInfo& info);
    MediaDeviceInfo playoutAudioDevice() const;
    std::vector<MediaDeviceInfo> recordingAudioDevices() const;
    std::vector<MediaDeviceInfo> playoutAudioDevices() const;
    double recordingVolume() const noexcept { return _recordingVolume; }
    double playoutVolume() const noexcept { return _playoutVolume; }
    void setRecordingVolumeVolume(double volume);
    void setPlayoutVolume(double volume);
    void setRecordingMute(bool mute);
    bool recordingMuted() const { return _recordingMuted; }
    void setPlayoutMute(bool mute);
    bool playoutMuted() const { return _playoutMuted; }
    std::unique_ptr<Session> createSession(Options options) const;
    static bool sslInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static bool wsaInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static std::unique_ptr<Impl> create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                                        const MicrophoneOptions& microphoneOptions,
                                        const std::shared_ptr<Bricks::Logger>& logger,
                                        bool logWebrtcEvents);
    // overrides of AdmProxyListener
    void onStarted(bool recording) final;
    void onMinMaxVolumeChanged(bool recording, uint32_t, uint32_t) final;
    void onDeviceChanged(bool recording, const MediaDeviceInfo& info);
protected:
    void updateAdmVolume(bool recording, double volume) const;
    void updateAdmMute(bool recording, bool mute) const;
    void updateRecordingVolume(double volume) const { updateAdmVolume(true, volume); }
    void updateRecordingMute(bool mute) const { updateAdmMute(true, mute); }
    void updateRecordingVolume() const { updateRecordingVolume(recordingVolume()); }
    void updateRecordingMute() const { updateRecordingMute(recordingMuted()); }
    void updatePlayoutVolume(double volume) const { updateAdmVolume(false, volume); }
    void updatePlayoutMute(bool mute) const { updateAdmMute(false, mute); }
    void updatePlayoutVolume() const { updatePlayoutVolume(playoutVolume()); }
    void updatePlayoutMute() const { updatePlayoutMute(playoutMuted()); }
    // final of Bricks::LoggableS<>
    std::string_view logCategory() const final { return g_logCategory; }
private:
    static void logPlatformDefects(const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    const std::shared_ptr<Websocket::Factory> _websocketsFactory;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    std::atomic<double> _recordingVolume = 0.2;
    std::atomic<double> _playoutVolume = 0.2;
    std::atomic_bool _recordingMuted;
    std::atomic_bool _playoutMuted;
};

Service::Service(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                 const MicrophoneOptions& microphoneOptions,
                 const std::shared_ptr<Bricks::Logger>& logger,
                 bool logWebrtcEvents)
    : _impl(Impl::create(websocketsFactory, microphoneOptions, logger, logWebrtcEvents))
{
}

Service::Service(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                 const std::shared_ptr<Bricks::Logger>& logger,
                 bool logWebrtcEvents)
    : Service(websocketsFactory, {}, logger, logWebrtcEvents)
{
}

Service::~Service()
{
}

ServiceState Service::state() const
{
    if (Impl::sslInitialized()) {
        if (Impl::wsaInitialized()) {
            if (_impl) {
                if (_impl->peerConnectionFactory()) {
                    return ServiceState::OK;
                }
                return ServiceState::WebRTCInitError;
            }
            return ServiceState::NoWebsoketsFactory;
        }
        return ServiceState::WSAFailure;
    }
    return ServiceState::SSLInitError;
}

std::unique_ptr<Session> Service::createSession(Options options) const
{
    return _impl->createSession(std::move(options));
}

MediaDeviceInfo Service::defaultRecordingAudioDevice() const
{
    return _impl->defaultRecordingAudioDevice();
}

MediaDeviceInfo Service::defaultPlayoutAudioDevice() const
{
    return _impl->defaultPlayoutAudioDevice();
}

bool Service::setRecordingAudioDevice(const MediaDeviceInfo& info)
{
    return _impl->setRecordingAudioDevice(info);
}

MediaDeviceInfo Service::recordingAudioDevice() const
{
    return _impl->recordingAudioDevice();
}

double Service::recordingVolume() const
{
    return _impl->recordingVolume();
}

void Service::setRecordingVolume(double volume)
{
    _impl->setRecordingVolumeVolume(volume);
}

bool Service::setPlayoutAudioDevice(const MediaDeviceInfo& info)
{
    return _impl->setPlayoutAudioDevice(info);
}

MediaDeviceInfo Service::playoutAudioDevice() const
{
    return _impl->playoutAudioDevice();
}

double Service::playoutVolume() const
{
    return _impl->playoutVolume();
}

void Service::setPlayoutVolume(double volume)
{
    _impl->setPlayoutVolume(volume);
}

void Service::setAudioRecording(bool recording)
{
    _impl->setRecordingMute(!recording);
}

bool Service::audioRecordingEnabled() const
{
    return !_impl->recordingMuted();
}

void Service::setAudioPlayout(bool playout)
{
    _impl->setPlayoutMute(!playout);
}

bool Service::audioPlayoutEnabled() const
{
    return !_impl->playoutMuted();
}

std::vector<MediaDeviceInfo> Service::recordingAudioDevices() const
{
    return _impl->recordingAudioDevices();
}

std::vector<MediaDeviceInfo> Service::playoutAudioDevices() const
{
    return _impl->playoutAudioDevices();
}

std::vector<MediaDeviceInfo> Service::cameraDevices() const
{
    return CameraManager::devices();
}

std::vector<CameraOptions> Service::cameraOptions(const MediaDeviceInfo& info) const
{
    if (const uint32_t number = CameraManager::capabilitiesNumber(info)) {
        std::vector<CameraOptions> options;
        options.reserve(number);
        for (uint32_t i = 0U; i < number; ++i) {
            webrtc::VideoCaptureCapability capability;
            if (CameraManager::capability(info, i, capability)) {
                options.push_back(map(capability));
            }
        }
        return options;
    }
    return {};
}

Service::Impl::Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                    const MicrophoneOptions& microphoneOptions,
                    const std::shared_ptr<Bricks::Logger>& logger,
                    bool logWebrtcEvents)
    : Bricks::LoggableS<AdmProxyListener>(logger)
    , _websocketsFactory(websocketsFactory)
    , _pcf(PeerConnectionFactory::create(true, microphoneOptions, logWebrtcEvents ? logger : nullptr))
{
    _recordingMuted = _playoutMuted = nullptr == _pcf;
    if (!_pcf) {
        logError("failed to create of peer connection factory");
    }
    else {
        _pcf->registerAdmRecordingListener(this, true);
        _pcf->registerAdmPlayoutListener(this, true);
        if (!_pcf->eventsQueue()) {
            logError("failed to create of events queue");
        }
        auto dev = _pcf->recordingAudioDevice();
        if (!dev.empty()) {
            logInfo("recording audio device is '" + dev._name + "'");
        }
        dev = _pcf->playoutAudioDevice();
        if (!dev.empty()) {
            logInfo("playout audio device is '" + dev._name + "'");
        }
    }
}

Service::Impl::~Impl()
{
    if (_pcf) {
        _pcf->registerAdmRecordingListener(this, false);
        _pcf->registerAdmPlayoutListener(this, false);
    }
}

MediaDeviceInfo Service::Impl::defaultRecordingAudioDevice() const
{
    if (_pcf) {
        return _pcf->defaultRecordingAudioDevice();
    }
    return {};
}

MediaDeviceInfo Service::Impl::defaultPlayoutAudioDevice() const
{
    if (_pcf) {
        return _pcf->defaultPlayoutAudioDevice();
    }
    return {};
}

bool Service::Impl::setRecordingAudioDevice(const MediaDeviceInfo& info)
{
    return _pcf && _pcf->setRecordingAudioDevice(info);
}

MediaDeviceInfo Service::Impl::recordingAudioDevice() const
{
    if (_pcf) {
        return _pcf->recordingAudioDevice();
    }
    return {};
}

bool Service::Impl::setPlayoutAudioDevice(const MediaDeviceInfo& info)
{
    return _pcf && _pcf->setPlayoutAudioDevice(info);
}

MediaDeviceInfo Service::Impl::playoutAudioDevice() const
{
    if (_pcf) {
        return _pcf->playoutAudioDevice();
    }
    return {};
}

std::vector<MediaDeviceInfo> Service::Impl::recordingAudioDevices() const
{
    if (_pcf) {
        return _pcf->recordingAudioDevices();
    }
    return {};
}

std::vector<MediaDeviceInfo> Service::Impl::playoutAudioDevices() const
{
    if (_pcf) {
        return _pcf->playoutAudioDevices();
    }
    return {};
}

void Service::Impl::setRecordingVolumeVolume(double volume)
{
    if (_pcf && exchangeVal(volume, _recordingVolume)) {
        updateRecordingVolume(volume);
    }
}

void Service::Impl::setPlayoutVolume(double volume)
{
    if (_pcf && exchangeVal(volume, _playoutVolume)) {
        updatePlayoutVolume(volume);
    }
}

void Service::Impl::setRecordingMute(bool mute)
{
    if (_pcf && exchangeVal(mute, _recordingMuted)) {
        updateRecordingMute(mute);
    }
}

void Service::Impl::setPlayoutMute(bool mute)
{
    if (_pcf && exchangeVal(mute, _playoutMuted)) {
        updatePlayoutMute(mute);
    }
}

std::unique_ptr<Session> Service::Impl::createSession(Options options) const
{
    std::unique_ptr<Session> session;
    if (_pcf) {
        if (auto socket = _websocketsFactory->create()) {
            session.reset(new Session(std::move(socket), _pcf.get(),
                                      std::move(options), logger()));
        }
    }
    return session;
}

bool Service::Impl::sslInitialized(const std::shared_ptr<Bricks::Logger>& logger)
{
    static const RtcInitializer initializer;
    if (!initializer && logger && logger->canLogError()) {
        logger->logError("Failed to SSL initialization", g_logCategory);
    }
    return initializer.initialized();
}

bool Service::Impl::wsaInitialized(const std::shared_ptr<Bricks::Logger>& logger)
{
#ifdef _WIN32
    static const WSAInitializer initializer;
    if (const auto error = initializer.GetError()) {
        if (logger && logger->canLogError()) {
            logger->logError("Failed to WINSOCK initialization, error code: " + std::to_string(error), g_logCategory);
        }
        return false;
    }
    if (logger && logger->canLogVerbose()) {
        const auto& wsaVersion = initializer.GetSelectedVersion();
        if (wsaVersion.has_value()) {
            logger->logVerbose("WINSOCK initialization is done, library version: " +
                              WSAInitializer::ToString(wsaVersion.value()), g_logCategory);
        }
    }
#endif
    return true;
}

std::unique_ptr<Service::Impl> Service::Impl::
    create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
           const MicrophoneOptions& microphoneOptions,
           const std::shared_ptr<Bricks::Logger>& logger,
           bool logWebrtcEvents)
{
    if (wsaInitialized(logger) && sslInitialized(logger) && websocketsFactory) {
        logPlatformDefects(logger);
        return std::make_unique<Impl>(websocketsFactory, microphoneOptions,
                                      logger, logWebrtcEvents);
    }
    return {};
}

void Service::Impl::onStarted(bool recording)
{
    if (recording) {
        updateRecordingVolume();
        updateRecordingMute();
    }
    else {
        updatePlayoutVolume();
        updatePlayoutMute();
    }
}

void Service::Impl::onMinMaxVolumeChanged(bool recording, uint32_t, uint32_t)
{
    if (recording) {
        updateRecordingVolume();
    }
    else {
        updatePlayoutVolume();
    }
}

void Service::Impl::onDeviceChanged(bool recording, const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        if (recording) {
            logInfo("recording audio device has been changed to '" + info._name + "'");
        }
        else {
            logInfo("playoud audio device has been changed to '" + info._name + "'");
        }
    }
}

void Service::Impl::updateAdmVolume(bool recording, double volume) const
{
    if (_pcf) {
        if (recording) {
            _pcf->setMicrophoneVolume(volume);
        }
        else {
            _pcf->setSpeakerVolume(volume);
        }
    }
}

void Service::Impl::updateAdmMute(bool recording, bool mute) const
{
    if (_pcf) {
        if (recording) {
            _pcf->setRecordingMute(mute);
        }
        else {
            _pcf->setPlayoutMute(mute);
        }
    }
}

void Service::Impl::logPlatformDefects(const std::shared_ptr<Bricks::Logger>& logger)
{
    if (logger && logger->canLogWarning()) {
#ifdef __APPLE__
        std::string envErrorInfo;
        const auto status = checkAppEnivonment(AESNoProblems, &envErrorInfo);
        if (AESNoProblems != status) {
            logger->logWarning("Some features of LiveKit SDK may be works incorrectly, see details below:", g_logCategory);
            if (testFlag<AESNoGuiThread>(status)) {
                logger->logWarning(" - main thread is not available", g_logCategory);
            }
            if (testFlag<AESNoInfoPlist>(status)) {
                logger->logWarning(" - application bundle info dictionary doesn't exist", g_logCategory);
            }
            if (testFlag<AESIncompleteInfoPlist>(status)) {
                logger->logWarning(" - application bundle info dictionary is incomplete", g_logCategory);
            }
            if (!envErrorInfo.empty()) {
                logger->logWarning(" - additional error info: " + envErrorInfo, g_logCategory);
            }
        }
#endif
    }
}

bool KeyProvider::setSharedKey(std::string_view key, const std::optional<uint8_t>& keyIndex)
{
    return setSharedKey(binaryFromString(std::move(key)), keyIndex);
}

bool KeyProvider::setKey(const std::string& identity, std::string_view key,
                         const std::optional<uint8_t>& keyIndex)
{
    return setKey(identity, binaryFromString(std::move(key)), keyIndex);
}

void KeyProvider::setSifTrailer(std::string_view trailer)
{
    setSifTrailer(binaryFromString(std::move(trailer)));
}

void KeyProviderOptions::setRatchetSalt(std::string_view salt)
{
    _ratchetSalt = binaryFromString(std::move(salt));
}

CameraOptions CameraOptions::defaultOptions()
{
    return map(CameraManager::defaultCapability());
}
#else
class Service::Impl {};

Service::Service(const std::shared_ptr<Websocket::Factory>&,
                 const MicrophoneOptions&,
                 const std::shared_ptr<Bricks::Logger>&, bool) {}

Service::Service(const std::shared_ptr<Websocket::Factory>&,
                 const std::shared_ptr<Bricks::Logger>&, bool) {}

Service::~Service() {}

ServiceState Service::state() const
{
    return ServiceState::NoWebRTC;
}

std::unique_ptr<Session> Service::createSession(Options options) const { return {}; }

MediaDeviceInfo Service::defaultRecordingAudioDevice() const { return {}; }

MediaDeviceInfo Service::defaultPlayoutAudioDevice() const { return {}; }

bool Service::setRecordingAudioDevice(const MediaDeviceInfo&) { return false; }

MediaDeviceInfo Service::recordingAudioDevice() const { return {}; }

double Service::recordingVolume() const { return 0.; }

void Service::setRecordingVolume(double) {}

bool Service::setPlayoutAudioDevice(const MediaDeviceInfo&) { return false; }

MediaDeviceInfo Service::playoutAudioDevice() const { return {}; }

double Service::playoutVolume() const { return 0.; }

void Service::setPlayoutVolume(double) {}

void Service::setAudioRecording(bool) {}

bool Service::audioRecordingEnabled() const { return false; }

void Service::setAudioPlayout(bool) {}

bool Service::audioPlayoutEnabled() const { return false; }

std::vector<MediaDeviceInfo> Service::recordingAudioDevices() const { return {}; }

std::vector<MediaDeviceInfo> Service::playoutAudioDevices() const { return {}; }

std::vector<MediaDeviceInfo> Service::cameraDevices() const { return {}; }

CameraOptions CameraOptions::defaultOptions() { return {}; }
#endif

NetworkType Service::activeNetworkType()
{
    return LiveKitCpp::activeNetworkType();
}

MediaAuthorizationLevel Service::mediaAuthorizationLevel()
{
    return LiveKitCpp::mediaAuthorizationLevel();
}

void Service::setMediaAuthorizationLevel(MediaAuthorizationLevel level)
{
    LiveKitCpp::setMediaAuthorizationLevel(level);
}

ClientInfo::ClientInfo()
{
    _protocol = LIVEKIT_PROTOCOL_VERSION;
}

ClientInfo ClientInfo::defaultClientInfo()
{
    ClientInfo ci;
    ci._os = operatingSystemName();
    ci._osVersion = operatingSystemVersion();
    ci._deviceModel = modelIdentifier();
    ci._network = toString(activeNetworkType());
    return ci;
}

} // namespace LiveKitCpp
