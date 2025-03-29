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
#include "AudioDeviceModuleListener.h"
#include "CameraManager.h"
#include "CameraVideoTrack.h"
#include "CameraTrackImpl.h"
#include "DefaultKeyProvider.h"
#include "LocalAudioTrack.h"
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
class Service::Impl : public Bricks::LoggableS<AudioDeviceModuleListener>
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
    std::unique_ptr<Session> createSession(Options options) const;
    std::shared_ptr<AudioTrack> createMicrophoneTrack() const;
    std::shared_ptr<CameraTrack> createCameraTrack(const CameraOptions& options) const;
    static bool sslInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static bool wsaInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static std::unique_ptr<Impl> create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                                        const MicrophoneOptions& microphoneOptions,
                                        const std::shared_ptr<Bricks::Logger>& logger,
                                        bool logWebrtcEvents);
    // overrides of AudioDeviceModuleListener
    void onRecordingChanged(const MediaDeviceInfo& info) final;
    void onPlayoutChanged(const MediaDeviceInfo& info) final;
protected:
    // final of Bricks::LoggableS<>
    std::string_view logCategory() const final { return g_logCategory; }
private:
    static void logPlatformDefects(const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    const std::shared_ptr<Websocket::Factory> _websocketsFactory;
    const MicrophoneOptions _microphoneOptions;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
};

Service::Service(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                 const MicrophoneOptions& microphoneOptions,
                 const std::shared_ptr<Bricks::Logger>& logger,
                 bool logWebrtcEvents)
    : _impl(Impl::create(websocketsFactory, microphoneOptions, logger, logWebrtcEvents))
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

bool Service::setPlayoutAudioDevice(const MediaDeviceInfo& info)
{
    return _impl->setPlayoutAudioDevice(info);
}

MediaDeviceInfo Service::playoutAudioDevice() const
{
    return _impl->playoutAudioDevice();
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

std::shared_ptr<AudioTrack> Service::createMicrophoneTrack() const
{
    return _impl->createMicrophoneTrack();
}

std::shared_ptr<CameraTrack> Service::createCameraTrack(const CameraOptions& options) const
{
    return _impl->createCameraTrack(options);
}

Service::Impl::Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                    const MicrophoneOptions& microphoneOptions,
                    const std::shared_ptr<Bricks::Logger>& logger,
                    bool logWebrtcEvents)
    : Bricks::LoggableS<AudioDeviceModuleListener>(logger)
    , _websocketsFactory(websocketsFactory)
    , _microphoneOptions(microphoneOptions)
    , _pcf(PeerConnectionFactory::Create(true, logWebrtcEvents ? logger : nullptr))
{
    if (!_pcf) {
        logError("failed to create of peer connection factory");
    }
    else {
        _pcf->addAdmListener(this);
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
        _pcf->removeAdmListener(this);
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

std::shared_ptr<AudioTrack> Service::Impl::createMicrophoneTrack() const
{
    if (_pcf) {
        
    }
    return {};
}

std::shared_ptr<CameraTrack> Service::Impl::createCameraTrack(const CameraOptions& options) const
{
    if (_pcf && CameraManager::available()) {
        auto rtcTrack = webrtc::make_ref_counted<CameraVideoTrack>(makeUuid(),
                                                                   _pcf->signalingThread(),
                                                                   map(options),
                                                                   logger());
    }
    return {};
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

void Service::Impl::onRecordingChanged(const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        logInfo("recording audio device has been changed to '" + info._name + "'");
    }
}

void Service::Impl::onPlayoutChanged(const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        logInfo("playoud audio device has been changed to '" + info._name + "'");
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

bool Service::setPlayoutAudioDevice(const MediaDeviceInfo&) { return false; }

MediaDeviceInfo Service::playoutAudioDevice() const { return {}; }

std::vector<MediaDeviceInfo> Service::recordingAudioDevices() const { return {}; }

std::vector<MediaDeviceInfo> Service::playoutAudioDevices() const { return {}; }

std::vector<MediaDeviceInfo> Service::cameraDevices() const { return {}; }

std::shared_ptr<AudioTrack> Service::createMicrophoneTrack() const { return {}; }

std::shared_ptr<CameraTrack> Service::createCameraTrack(const CameraOptions&) const
{
    return {};
}

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
