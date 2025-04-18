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
#include "livekit/rtc/Service.h"
#include "AdmProxyListener.h"
#include "AdmProxyFacade.h"
#include "AsyncCameraSourceImpl.h"
#include "CameraManager.h"
#include "DefaultKeyProvider.h"
#include "DesktopConfiguration.h"
#include "Listeners.h"
#include "LocalVideoDeviceImpl.h"
#include "LocalWebRtcTrack.h"
#include "Loggable.h"
#include "Logger.h"
#include "MediaAuthorization.h"
#include "MicAudioDevice.h"
#include "PeerConnectionFactory.h"
#include "RtcInitializer.h"
#include "WebsocketEndPoint.h"
#include "WebsocketFactory.h"
#include "VolumeControl.h"
#include "Utils.h"
#include "livekit/signaling/NetworkType.h"
#include "livekit/rtc/e2e/KeyProviderOptions.h"
#include "livekit/rtc/e2e/KeyProvider.h"
#include "livekit/rtc/media/VideoOptions.h"
#include "livekit/rtc/media/VideoFrame.h"
#include "livekit/rtc/ServiceListener.h"
#ifdef __APPLE__
#include "AppEnvironment.h"
#elif defined(_WIN32)
#include "WSAInitializer.h"
#endif

namespace {

const std::string_view g_logCategory("service");

using namespace LiveKitCpp;

class AsyncCameraSource : public AsyncVideoSource
{
public:
    AsyncCameraSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                      const std::shared_ptr<Bricks::Logger>& logger);
    // impl. of webrtc::VideoTrackSourceInterface
    bool is_screencast() const final { return false;}
};

}

namespace LiveKitCpp
{

class Service::Impl : public Bricks::LoggableS<AdmProxyListener>
{
public:
    Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
         const std::shared_ptr<Bricks::Logger>& logger,
         bool logWebrtcEvents);
    ~Impl();
    const auto& websocketsFactory() const noexcept { return _websocketsFactory; }
    const auto& peerConnectionFactory() const noexcept { return _pcf; }
    std::shared_ptr<AudioDevice> createMicrophone(const AudioRecordingOptions& options) const;
    std::shared_ptr<LocalVideoDevice> createCamera(MediaDeviceInfo info, VideoOptions options) const;
    MediaDeviceInfo defaultAudioRecordingDevice() const;
    MediaDeviceInfo defaultAudioPlayoutDevice() const;
    bool setAudioRecordingDevice(const MediaDeviceInfo& info);
    MediaDeviceInfo recordingAudioDevice() const;
    bool setAudioPlayoutDevice(const MediaDeviceInfo& info);
    MediaDeviceInfo playoutAudioDevice() const;
    std::vector<MediaDeviceInfo> screens() const { return _desktopConfiguration.enumerateScreens(); }
    std::vector<MediaDeviceInfo> windows() const { return _desktopConfiguration.enumerateWindows(); }
    std::vector<MediaDeviceInfo> recordingAudioDevices() const;
    std::vector<MediaDeviceInfo> playoutAudioDevices() const;
    double recordingAudioVolume() const noexcept;
    double playoutAudioVolume() const noexcept;
    void setRecordingAudioVolume(double volume);
    void setPlayoutAudioVolume(double volume);
    void setRecordingMute(bool mute);
    bool recordingMuted() const { return _recordingMuted; }
    void setPlayoutMute(bool mute);
    bool playoutMuted() const { return _playoutMuted; }
    std::unique_ptr<Session> createSession(Options options) const;
    void addListener(ServiceListener* listener);
    void removeListener(ServiceListener* listener);
    static bool sslInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static bool wsaInitialized(const std::shared_ptr<Bricks::Logger>& logger = {});
    static std::unique_ptr<Impl> create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                                        const std::shared_ptr<Bricks::Logger>& logger,
                                        bool logWebrtcEvents);
    // overrides of AdmProxyListener
    void onStarted(bool recording) final;
    void onStopped(bool recording) final;
    void onMuteChanged(bool recording, bool mute) final;
    void onVolumeChanged(bool recording, uint32_t volume) final;
    void onMinMaxVolumeChanged(bool recording, uint32_t minV, uint32_t maxV) final;
    void onDeviceChanged(bool recording, const MediaDeviceInfo& info);
protected:
    // final of Bricks::LoggableS<>
    std::string_view logCategory() const final { return g_logCategory; }
private:
    webrtc::scoped_refptr<LocalWebRtcTrack> createCameraTrack() const;
    uint32_t recordingDevAudioVolume() const;
    uint32_t playoutDevAudioVolume() const noexcept;
    void updateAdmVolume(bool recording, uint32_t volume) const;
    void updateAdmMute(bool recording, bool mute) const;
    void updateRecordingVolume(uint32_t volume) const { updateAdmVolume(true, volume); }
    void updateRecordingMute(bool mute) const { updateAdmMute(true, mute); }
    void updatePlayoutVolume(uint32_t volume) const { updateAdmVolume(false, volume); }
    void updatePlayoutMute(bool mute) const { updateAdmMute(false, mute); }
    static void logPlatformDefects(const std::shared_ptr<Bricks::Logger>& logger = {});
private:
    static inline const VolumeControl _defaultRecording = {70U, 0U, 255U};
    static inline const VolumeControl _defaultPlayout = {51U, 0U, 255U};
    const std::shared_ptr<Websocket::Factory> _websocketsFactory;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    const DesktopConfiguration _desktopConfiguration;
    Bricks::SafeObj<VolumeControl> _recordingVolume;
    Bricks::SafeObj<VolumeControl> _playoutVolume;
    std::atomic_bool _recordingMuted;
    std::atomic_bool _playoutMuted;
    Bricks::Listeners<ServiceListener*> _listeners;
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

std::unique_ptr<Session> Service::createSession(Options options) const
{
    if (_impl) {
        return _impl->createSession(std::move(options));
    }
    return {};
}

std::shared_ptr<AudioDevice> Service::createMicrophone(const AudioRecordingOptions& options) const
{
    if (_impl) {
        return _impl->createMicrophone(options);
    }
    return {};
}

std::shared_ptr<LocalVideoDevice> Service::createCamera(MediaDeviceInfo info, VideoOptions options) const
{
    if (_impl) {
        return _impl->createCamera(std::move(info), std::move(options));
    }
    return {};
}

MediaDeviceInfo Service::defaultAudioRecordingDevice() const
{
    if (_impl) {
        return _impl->defaultAudioRecordingDevice();
    }
    return {};
}

MediaDeviceInfo Service::defaultAudioPlayoutDevice() const
{
    if (_impl) {
        return _impl->defaultAudioPlayoutDevice();
    }
    return {};
}

bool Service::setAudioRecordingDevice(const MediaDeviceInfo& info)
{
    return _impl && _impl->setAudioRecordingDevice(info);
}

MediaDeviceInfo Service::recordingAudioDevice() const
{
    if (_impl) {
        return _impl->recordingAudioDevice();
    }
    return {};
}

double Service::recordingAudioVolume() const
{
    if (_impl) {
        return _impl->recordingAudioVolume();
    }
    return 0.;
}

void Service::setRecordingAudioVolume(double volume)
{
    if (_impl) {
        _impl->setRecordingAudioVolume(volume);
    }
}

bool Service::setAudioPlayoutDevice(const MediaDeviceInfo& info)
{
    return _impl && _impl->setAudioPlayoutDevice(info);
}

MediaDeviceInfo Service::playoutAudioDevice() const
{
    if (_impl) {
        return _impl->playoutAudioDevice();
    }
    return {};
}

double Service::playoutAudioVolume() const
{
    if (_impl) {
        return _impl->playoutAudioVolume();
    }
    return 0.;
}

void Service::setPlayoutAudioVolume(double volume)
{
    _impl->setPlayoutAudioVolume(volume);
}

void Service::setAudioRecordingEnabled(bool enabled)
{
    if (_impl) {
        _impl->setRecordingMute(!enabled);
    }
}

bool Service::audioRecordingEnabled() const
{
    return _impl && !_impl->recordingMuted();
}

void Service::setAudioPlayoutEnabled(bool enabled)
{
    if (_impl) {
        _impl->setPlayoutMute(!enabled);
    }
}

bool Service::audioPlayoutEnabled() const
{
    return _impl && !_impl->playoutMuted();
}

std::vector<MediaDeviceInfo> Service::screens() const
{
    if (_impl) {
        return _impl->screens();
    }
    return {};
}

std::vector<MediaDeviceInfo> Service::windows() const
{
    if (_impl) {
        return _impl->windows();
    }
    return {};
}

std::vector<MediaDeviceInfo> Service::recordingAudioDevices() const
{
    if (_impl) {
        return _impl->recordingAudioDevices();
    }
    return {};
}

std::vector<MediaDeviceInfo> Service::playoutAudioDevices() const
{
    if (_impl) {
        return _impl->playoutAudioDevices();
    }
    return {};
}

std::vector<MediaDeviceInfo> Service::cameraDevices() const
{
    if (_impl) {
        return CameraManager::devices();
    }
    return {};
}

std::vector<VideoOptions> Service::cameraOptions(const MediaDeviceInfo& info) const
{
    if (_impl) {
        if (const uint32_t number = CameraManager::capabilitiesNumber(info)) {
            std::vector<VideoOptions> options;
            options.reserve(number);
            for (uint32_t i = 0U; i < number; ++i) {
                webrtc::VideoCaptureCapability capability;
                if (CameraManager::capability(info, i, capability)) {
                    options.push_back(map(capability));
                }
            }
            return options;
        }
    }
    return {};
}

void Service::addListener(ServiceListener* listener)
{
    if (_impl) {
        _impl->addListener(listener);
    }
}

void Service::removeListener(ServiceListener* listener)
{
    if (_impl) {
        _impl->removeListener(listener);
    }
}


VideoOptions Service::defaultCameraOptions()
{
    return map(CameraManager::defaultCapability());
}

Service::Impl::Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                    const std::shared_ptr<Bricks::Logger>& logger,
                    bool logWebrtcEvents)
    : Bricks::LoggableS<AdmProxyListener>(logger)
    , _websocketsFactory(websocketsFactory)
    , _pcf(PeerConnectionFactory::create(true, logWebrtcEvents ? logger : nullptr))
    , _recordingVolume(_defaultRecording)
    , _playoutVolume(_defaultPlayout)
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
        if (!_desktopConfiguration.screensEnumerationIsAvailable()) {
            logWarning("screens enumeration is not available");
        }
        if (!_desktopConfiguration.windowsEnumerationIsAvailable()) {
            logWarning("windows enumeration is not available");
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

std::shared_ptr<AudioDevice> Service::Impl::createMicrophone(const AudioRecordingOptions& options) const
{
    if (_pcf) {
        return MicAudioDevice::create(_pcf.get(), options, logger());
    }
    return {};
}

std::shared_ptr<LocalVideoDevice> Service::Impl::createCamera(MediaDeviceInfo info, VideoOptions options) const
{
    if (auto track = createCameraTrack()) {
        track->setOptions(std::move(options));
        track->setDeviceInfo(std::move(info));
        return std::make_shared<LocalVideoDeviceImpl>(std::move(track));
    }
    return {};
}

MediaDeviceInfo Service::Impl::defaultAudioRecordingDevice() const
{
    if (_pcf) {
        return _pcf->defaultAudioRecordingDevice();
    }
    return {};
}

MediaDeviceInfo Service::Impl::defaultAudioPlayoutDevice() const
{
    if (_pcf) {
        return _pcf->defaultAudioPlayoutDevice();
    }
    return {};
}

bool Service::Impl::setAudioRecordingDevice(const MediaDeviceInfo& info)
{
    return _pcf && _pcf->setAudioRecordingDevice(info);
}

MediaDeviceInfo Service::Impl::recordingAudioDevice() const
{
    if (_pcf) {
        return _pcf->recordingAudioDevice();
    }
    return {};
}

bool Service::Impl::setAudioPlayoutDevice(const MediaDeviceInfo& info)
{
    return _pcf && _pcf->setAudioPlayoutDevice(info);
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

double Service::Impl::recordingAudioVolume() const noexcept
{
    LOCK_READ_SAFE_OBJ(_recordingVolume);
    return _recordingVolume->normalizedVolume();
}

double Service::Impl::playoutAudioVolume() const noexcept
{
    LOCK_READ_SAFE_OBJ(_playoutVolume);
    return _playoutVolume->normalizedVolume();
}

void Service::Impl::setRecordingAudioVolume(double normalizedVolume)
{
    if (_pcf) {
        std::optional<uint32_t> volume;
        {
            LOCK_WRITE_SAFE_OBJ(_recordingVolume);
            if (_recordingVolume->setNormalizedVolume(normalizedVolume)) {
                volume = _recordingVolume->volume();
            }
        }
        if (volume.has_value()) {
            updateRecordingVolume(volume.value());
        }
    }
}

void Service::Impl::setPlayoutAudioVolume(double normalizedVolume)
{
    if (_pcf) {
        std::optional<uint32_t> volume;
        {
            LOCK_WRITE_SAFE_OBJ(_playoutVolume);
            if (_playoutVolume->setNormalizedVolume(normalizedVolume)) {
                volume = _playoutVolume->volume();
            }
        }
        if (volume.has_value()) {
            updatePlayoutVolume(volume.value());
        }
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

void Service::Impl::addListener(ServiceListener* listener)
{
    _listeners.add(listener);
}

void Service::Impl::removeListener(ServiceListener* listener)
{
    _listeners.remove(listener);
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
           const std::shared_ptr<Bricks::Logger>& logger,
           bool logWebrtcEvents)
{
    if (wsaInitialized(logger) && sslInitialized(logger) && websocketsFactory) {
        logPlatformDefects(logger);
        return std::make_unique<Impl>(websocketsFactory, logger, logWebrtcEvents);
    }
    return {};
}

void Service::Impl::onStarted(bool recording)
{
    if (recording) {
        updateRecordingVolume(recordingDevAudioVolume());
        updateRecordingMute(recordingMuted());
        _listeners.invoke(&ServiceListener::onAudioRecordingStarted);
    }
    else {
        updatePlayoutVolume(playoutDevAudioVolume());
        updatePlayoutMute(playoutMuted());
        _listeners.invoke(&ServiceListener::onAudioPlayoutStarted);
    }
}

void Service::Impl::onStopped(bool recording)
{
    if (recording) {
        _listeners.invoke(&ServiceListener::onAudioRecordingStopped);
    }
    else {
        _listeners.invoke(&ServiceListener::onAudioPlayoutStopped);
    }
}

void Service::Impl::onMuteChanged(bool recording, bool mute)
{
    if (recording) {
        _listeners.invoke(&ServiceListener::onAudioRecordingEnabled, !mute);
    }
    else {
        _listeners.invoke(&ServiceListener::onAudioPlayoutEnabled, !mute);
    }
}

void Service::Impl::onVolumeChanged(bool recording, uint32_t volume)
{
    std::optional<double> normalized;
    if (recording) {
        LOCK_WRITE_SAFE_OBJ(_recordingVolume);
        if (_recordingVolume->setVolume(volume)) {
            normalized = _recordingVolume->normalizedVolume();
        }
    }
    else {
        LOCK_WRITE_SAFE_OBJ(_playoutVolume);
        if (_playoutVolume->setVolume(volume)) {
            normalized = _playoutVolume->normalizedVolume();
        }
    }
    if (normalized.has_value()) {
        if (recording) {
            _listeners.invoke(&ServiceListener::onAudioRecordingVolumeChanged, normalized.value());
        }
        else {
            _listeners.invoke(&ServiceListener::onAudioPlayoutVolumeChanged, normalized.value());
        }
    }
}

void Service::Impl::onMinMaxVolumeChanged(bool recording,
                                          uint32_t minVolume,
                                          uint32_t maxVolume)
{
    std::optional<double> normalized;
    if (recording) {
        LOCK_WRITE_SAFE_OBJ(_recordingVolume);
        if (_recordingVolume->setRange(minVolume, maxVolume)) {
            normalized = _recordingVolume->normalizedVolume();
        }
    }
    else {
        LOCK_WRITE_SAFE_OBJ(_playoutVolume);
        if (_playoutVolume->setRange(minVolume, maxVolume)) {
            normalized = _playoutVolume->normalizedVolume();
        }
    }
    if (normalized.has_value()) {
        if (recording) {
            _listeners.invoke(&ServiceListener::onAudioRecordingVolumeChanged, normalized.value());
        }
        else {
            _listeners.invoke(&ServiceListener::onAudioPlayoutVolumeChanged, normalized.value());
        }
    }
}

void Service::Impl::onDeviceChanged(bool recording, const MediaDeviceInfo& info)
{
    if (!info.empty()) {
        if (recording) {
            logInfo("recording audio device has been changed to '" + info._name + "'");
            _listeners.invoke(&ServiceListener::onAudioRecordingDeviceChanged, info);
        }
        else {
            logInfo("playoud audio device has been changed to '" + info._name + "'");
            _listeners.invoke(&ServiceListener::onAudioPlayoutDeviceChanged, info);
        }
    }
}

webrtc::scoped_refptr<LocalWebRtcTrack> Service::Impl::createCameraTrack() const
{
    if (_pcf) {
        auto source = webrtc::make_ref_counted<AsyncCameraSource>(_pcf->signalingThread(), logger());
        return webrtc::make_ref_counted<LocalWebRtcTrack>(makeUuid(), std::move(source));
    }
    return {};
}

uint32_t Service::Impl::recordingDevAudioVolume() const
{
    LOCK_READ_SAFE_OBJ(_recordingVolume);
    return _recordingVolume->volume();
}

uint32_t Service::Impl::playoutDevAudioVolume() const noexcept
{
    LOCK_READ_SAFE_OBJ(_playoutVolume);
    return _playoutVolume->volume();
}

void Service::Impl::updateAdmVolume(bool recording, uint32_t volume) const
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

KeyProviderOptions KeyProviderOptions::defaultOptions()
{
    KeyProviderOptions options;
    options._sharedKey = false;
    options.setRatchetSalt("LKFrameEncryptionKey");
    options._ratchetWindowSize = 8;
    options._keyRingSize = 16;
    options._failureTolerance = 10;
    return options;
}

} // namespace LiveKitCpp


namespace
{

AsyncCameraSource::AsyncCameraSource(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                     const std::shared_ptr<Bricks::Logger>& logger)
    : AsyncVideoSource(std::make_shared<AsyncCameraSourceImpl>(std::move(signalingQueue), logger))
{
}

}
