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
#include "Room.h"
#include "Logger.h"
#include "MediaAuthorization.h"
#include "WebsocketEndPoint.h"
#include "WebsocketFactory.h"
#include "NetworkType.h"
#include "Utils.h"
#include "rtc/ClientInfo.h"
#ifdef WEBRTC_AVAILABLE
#include "AudioDeviceModuleListener.h"
#include "CameraManager.h"
#include "Loggable.h"
#include "PeerConnectionFactory.h"
#include "SSLInitiallizer.h"
#ifdef __APPLE__
#include "AppEnvironment.h"
#elif defined(_WIN32)
#include "WSAInitializer.h"
#endif
#endif

#ifdef WEBRTC_AVAILABLE
namespace {

const std::string_view g_logCategory("service");

}
#endif

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE
class Service::Impl : public Bricks::LoggableS<AudioDeviceModuleListener>
{
public:
    Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
         const std::shared_ptr<Bricks::Logger>& logger, bool logWebrtcEvents);
    ~Impl();
    const auto& websocketsFactory() const noexcept { return _websocketsFactory; }
    const auto& peerConnectionFactory() const noexcept { return _pcf; }
    MediaDevice defaultRecordingAudioDevice() const;
    MediaDevice defaultPlayoutAudioDevice() const;
    bool setRecordingAudioDevice(const MediaDevice& device);
    MediaDevice recordingAudioDevice() const;
    bool setPlayoutAudioDevice(const MediaDevice& device);
    MediaDevice playoutAudioDevice() const;
    std::vector<MediaDevice> recordingAudioDevices() const;
    std::vector<MediaDevice> playoutAudioDevices() const;
    template <typename TOutput>
    TOutput makeRoom(const Options& signalOptions) const;
    static bool sslInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static bool wsaInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static std::unique_ptr<Impl> create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                                        const std::shared_ptr<Bricks::Logger>& logger,
                                        bool logWebrtcEvents);
    // overrides of AudioDeviceModuleListener
    void onRecordingChanged(const MediaDevice& device) final;
    void onPlayoutChanged(const MediaDevice& device) final;
protected:
    // final of Bricks::LoggableS<>
    std::string_view logCategory() const final { return g_logCategory; }
private:
    static void logPlatformDefects(const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    const std::shared_ptr<Websocket::Factory> _websocketsFactory;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
};

Service::Service(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                 const std::shared_ptr<Bricks::Logger>& logger,
                 bool logWebrtcEvents)
    : _impl(Impl::create(websocketsFactory, logger, logWebrtcEvents))
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

Room* Service::makeRoom(const Options& signalOptions) const
{
    return _impl->makeRoom<Room*>(signalOptions);
}

std::shared_ptr<Room> Service::makeRoomS(const Options& signalOptions) const
{
    return _impl->makeRoom<std::shared_ptr<Room>>(signalOptions);
}

std::unique_ptr<Room> Service::makeRoomU(const Options& signalOptions) const
{
    return _impl->makeRoom<std::unique_ptr<Room>>(signalOptions);
}

MediaDevice Service::defaultRecordingCameraDevice() const
{
    MediaDevice dev;
    if (CameraManager::defaultDevice(dev)) {
        return dev;
    }
    return {};
}

MediaDevice Service::defaultRecordingAudioDevice() const
{
    return _impl->defaultRecordingAudioDevice();
}

MediaDevice Service::defaultPlayoutAudioDevice() const
{
    return _impl->defaultPlayoutAudioDevice();
}

bool Service::setRecordingAudioDevice(const MediaDevice& device)
{
    return _impl->setRecordingAudioDevice(device);
}

MediaDevice Service::recordingAudioDevice() const
{
    return _impl->recordingAudioDevice();
}

bool Service::setPlayoutAudioDevice(const MediaDevice& device)
{
    return _impl->setPlayoutAudioDevice(device);
}

MediaDevice Service::playoutAudioDevice() const
{
    return _impl->playoutAudioDevice();
}

std::vector<MediaDevice> Service::recordingAudioDevices() const
{
    return _impl->recordingAudioDevices();
}

std::vector<MediaDevice> Service::playoutAudioDevices() const
{
    return _impl->playoutAudioDevices();
}

std::vector<MediaDevice> Service::recordingCameraDevices() const
{
    return CameraManager::devices();
}

Service::Impl::Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                    const std::shared_ptr<Bricks::Logger>& logger, bool logWebrtcEvents)
    : Bricks::LoggableS<AudioDeviceModuleListener>(logger)
    , _websocketsFactory(websocketsFactory)
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

MediaDevice Service::Impl::defaultRecordingAudioDevice() const
{
    if (_pcf) {
        return _pcf->defaultRecordingAudioDevice();
    }
    return {};
}

MediaDevice Service::Impl::defaultPlayoutAudioDevice() const
{
    if (_pcf) {
        return _pcf->defaultPlayoutAudioDevice();
    }
    return {};
}

bool Service::Impl::setRecordingAudioDevice(const MediaDevice& device)
{
    return _pcf && _pcf->setRecordingAudioDevice(device);
}

MediaDevice Service::Impl::recordingAudioDevice() const
{
    if (_pcf) {
        return _pcf->recordingAudioDevice();
    }
    return {};
}

bool Service::Impl::setPlayoutAudioDevice(const MediaDevice& device)
{
    return _pcf && _pcf->setPlayoutAudioDevice(device);
}

MediaDevice Service::Impl::playoutAudioDevice() const
{
    if (_pcf) {
        return _pcf->playoutAudioDevice();
    }
    return {};
}

std::vector<MediaDevice> Service::Impl::recordingAudioDevices() const
{
    if (_pcf) {
        return _pcf->recordingAudioDevices();
    }
    return {};
}

std::vector<MediaDevice> Service::Impl::playoutAudioDevices() const
{
    if (_pcf) {
        return _pcf->playoutAudioDevices();
    }
    return {};
}

template <typename TOutput>
TOutput Service::Impl::makeRoom(const Options& signalOptions) const
{
    if (_pcf) {
        if (auto socket = _websocketsFactory->create()) {
            return TOutput(new Room(std::move(socket), _pcf.get(),
                                    signalOptions, logger()));
        }
    }
    return TOutput(nullptr);
}

bool Service::Impl::sslInitialized(const std::shared_ptr<Bricks::Logger>& logger)
{
    static const SSLInitiallizer initializer;
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
           const std::shared_ptr<Bricks::Logger>& logger,
           bool logWebrtcEvents)
{
    if (wsaInitialized(logger) && sslInitialized(logger) && websocketsFactory) {
        logPlatformDefects(logger);
        return std::make_unique<Impl>(websocketsFactory, logger, logWebrtcEvents);
    }
    return {};
}

void Service::Impl::onRecordingChanged(const MediaDevice& device)
{
    if (!device.empty()) {
        logInfo("recording audio device has been changed to '" + device._name + "'");
    }
}

void Service::Impl::onPlayoutChanged(const MediaDevice& device)
{
    if (!device.empty()) {
        logInfo("playoud audio device has been changed to '" + device._name + "'");
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

#else
class Service::Impl {};

Service::Service(const std::shared_ptr<Websocket::Factory>&,
                               const std::shared_ptr<Bricks::Logger>&, bool) {}

Service::~Service() {}

ServiceState Service::state() const
{
    return ServiceState::NoWebRTC;
}

Room* Service::makeRoom(const Options&) const { return nullptr; }

std::shared_ptr<Room> Service::makeRoomS(const Options&) const { return {}; }

std::unique_ptr<Room> Service::makeRoomU(const Options&) const { return {}; }

MediaDevice Service::defaultRecordingCameraDevice() const { return {}; }

MediaDevice Service::defaultRecordingAudioDevice() const { return {}; }

MediaDevice Service::defaultPlayoutAudioDevice() const { return {}; }

bool Service::setRecordingAudioDevice(const MediaDevice&) { return false; }

MediaDevice Service::recordingAudioDevice() const { return {}; }

bool Service::setPlayoutAudioDevice(const MediaDevice&) { return false; }

MediaDevice Service::playoutAudioDevice() const { return {}; }

std::vector<MediaDevice> Service::recordingAudioDevices() const { return {}; }

std::vector<MediaDevice> Service::playoutAudioDevices() const { return {}; }

std::vector<MediaDevice> Service::recordingCameraDevices() const { return {}; }
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
