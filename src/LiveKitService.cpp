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
#include "LiveKitService.h"
#include "LiveKitRoom.h"
#include "Logger.h"
#include "WebsocketEndPoint.h"
#include "WebsocketFactory.h"
#include "Utils.h"
#ifdef WEBRTC_AVAILABLE
#include "PeerConnectionFactory.h"
#ifdef __APPLE__
#include "AppEnvironment.h"
#endif
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>
#include <rtc_base/crypto_random.h>
#include <rtc_base/time_utils.h>
#include <system_wrappers/include/field_trial.h>
#include <libyuv/cpu_id.h>
#ifdef _WIN32
#include <optional>
#include <rtc_base/win32.h>
#endif
#endif

namespace {

#ifdef WEBRTC_AVAILABLE

static const std::string_view g_logCategory("LiveKitService_Init");

class SSLInitiallizer
{
public:
    SSLInitiallizer();
    ~SSLInitiallizer();
    bool initialized() const noexcept { return _sslInitialized; }
    explicit operator bool() const { return initialized(); }
private:
    const bool _sslInitialized;
};

#ifdef _WIN32
class WSAInitializer
{
public:
    enum class Version : WORD
    {
        v1_0 = MAKEWORD(1, 0),
        v1_1 = MAKEWORD(1, 1),
        v2_0 = MAKEWORD(2, 0),
        v2_1 = MAKEWORD(2, 1),
        v2_2 = MAKEWORD(2, 2)
    };
public:
    WSAInitializer();
    ~WSAInitializer();
    int GetError() const noexcept { return _error; }
    const auto& GetSelectedVersion() const noexcept { return _selectedVersion; }
    static std::string ToString(Version version);
private:
    static int WsaStartup(Version version, WSADATA& wsaData);
private:
    int _error = 0;
    std::optional<Version> _selectedVersion;
};
#endif
#endif

}

namespace LiveKitCpp
{
#ifdef WEBRTC_AVAILABLE
class LiveKitService::Impl
{
public:
    Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
         const std::shared_ptr<Logger>& logger);
    const auto& websocketsFactory() const noexcept { return _websocketsFactory; }
    const auto& peerConnectionFactory() const noexcept { return _pcf; }
    template <typename TOutput>
    TOutput makeRoom(const ConnectOptions& connectOptions,
                     const RoomOptions& roomOptions) const;
    static bool sslInitialized(const std::shared_ptr<Logger>& logger = {});
    static bool wsaInitialized(const std::shared_ptr<Logger>& logger = {});
    static std::unique_ptr<Impl> create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                                        const std::shared_ptr<Logger>& logger);
private:
    static void logPlatformDefects(const std::shared_ptr<Logger>& logger = {});
private:
    const std::shared_ptr<Websocket::Factory> _websocketsFactory;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
};

LiveKitService::LiveKitService(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                               const std::shared_ptr<Logger>& logger)
    : _impl(Impl::create(websocketsFactory, logger))
{
}

LiveKitService::~LiveKitService()
{
}

LiveKitServiceState LiveKitService::state() const
{
    if (Impl::sslInitialized()) {
        if (Impl::wsaInitialized()) {
            if (_impl) {
                if (_impl->peerConnectionFactory()) {
                    return LiveKitServiceState::OK;
                }
                return LiveKitServiceState::WebRTCInitError;
            }
            return LiveKitServiceState::NoWebsoketsFactory;
        }
        return LiveKitServiceState::WSAFailure;
    }
    return LiveKitServiceState::SSLInitError;
}

LiveKitRoom* LiveKitService::makeRoom(const ConnectOptions& connectOptions,
                                      const RoomOptions& roomOptions) const
{
    return _impl->makeRoom<LiveKitRoom*>(connectOptions, roomOptions);
}

std::shared_ptr<LiveKitRoom> LiveKitService::makeRoomS(const ConnectOptions& connectOptions,
                                                       const RoomOptions& roomOptions) const
{
    return _impl->makeRoom<std::shared_ptr<LiveKitRoom>>(connectOptions, roomOptions);
}

std::unique_ptr<LiveKitRoom> LiveKitService::makeRoomU(const ConnectOptions& connectOptions,
                                                       const RoomOptions& roomOptions) const
{
    return _impl->makeRoom<std::unique_ptr<LiveKitRoom>>(connectOptions, roomOptions);
}

LiveKitService::Impl::Impl(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
                           const std::shared_ptr<Logger>& logger)
    : _websocketsFactory(websocketsFactory)
    , _pcf(PeerConnectionFactory::Create(true, true, logger))
{
}

template <typename TOutput>
TOutput LiveKitService::Impl::makeRoom(const ConnectOptions& connectOptions,
                                       const RoomOptions& roomOptions) const
{
    if (_pcf) {
        if (auto socket = _websocketsFactory->create()) {
            return TOutput(new LiveKitRoom(std::move(socket), _pcf.get(),
                                           connectOptions, roomOptions));
        }
    }
    return TOutput(nullptr);
}

bool LiveKitService::Impl::sslInitialized(const std::shared_ptr<Logger>& logger)
{
    static const SSLInitiallizer initializer;
    if (!initializer && logger && logger->canLogError()) {
        logger->logError("Failed to SSL initialization", g_logCategory);
    }
    return initializer.initialized();
}

bool LiveKitService::Impl::wsaInitialized(const std::shared_ptr<Logger>& logger)
{
#ifdef _WIN32
    static const WSAInitializer initializer;
    if (const auto error = initializer.GetError()) {
        if (logger && logger->canLogError()) {
            logger->logError("Failed to WINSOCK initialization, error code: " + std::to_string(error), g_srvInit);
        }
        return false;
    }
    if (logger && && logger->canLogVerbose()) {
        const auto& wsaVersion = initializer.GetSelectedVersion();
        if (wsaVersion.has_value()) {
            logger->logVerbose("WINSOCK initialization is done, library version: " +
                              WSAInitializer::ToString(wsaVersion.value()), g_srvInit);
        }
    }
#endif
    return true;
}

std::unique_ptr<LiveKitService::Impl> LiveKitService::Impl::
    create(const std::shared_ptr<Websocket::Factory>& websocketsFactory,
           const std::shared_ptr<Logger>& logger)
{
    if (wsaInitialized(logger) && sslInitialized(logger) && websocketsFactory) {
        logPlatformDefects(logger);
        return std::make_unique<Impl>(websocketsFactory, logger);
    }
    return {};
}

void LiveKitService::Impl::logPlatformDefects(const std::shared_ptr<Logger>& logger)
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
class LiveKitService::Impl {};

LiveKitService::LiveKitService(const std::shared_ptr<Websocket::Factory>&,
                               const std::shared_ptr<Logger>&) {}

LiveKitService::~LiveKitService() {}

LiveKitServiceState LiveKitService::state() const
{
    return LiveKitServiceState::NoWebRTC;
}

LiveKitRoom* LiveKitService::makeRoom(const RoomOptions&) const { return nullptr; }

std::shared_ptr<LiveKitRoom> LiveKitService::makeRoomS(const RoomOptions&) const { return {}; }

std::unique_ptr<LiveKitRoom> LiveKitService::makeRoomU(const RoomOptions&) const { return {}; }

#endif

ConnectOptions::ConnectOptions()
{
    _protocolVersion = LIVEKIT_PROTOCOL_VERSION;
}

} // namespace LiveKitCpp

namespace {

#ifdef WEBRTC_AVAILABLE
SSLInitiallizer::SSLInitiallizer()
    : _sslInitialized(rtc::InitializeSSL())
{
    if (_sslInitialized) {
        rtc::LogMessage::LogToDebug(rtc::LS_NONE);
        rtc::LogMessage::LogTimestamps(false);
        rtc::LogMessage::SetLogToStderr(false);
        libyuv::MaskCpuFlags(-1); // to enable all cpu specific optimizations
        rtc::InitRandom(static_cast<int>(rtc::Time()));
        webrtc::field_trial::InitFieldTrialsFromString("WebRTC-SupportVP9SVC/EnabledByFlag_3SL3TL/");
    }
}

SSLInitiallizer::~SSLInitiallizer()
{
    if (_sslInitialized) {
        rtc::CleanupSSL();
    }
}

#ifdef _WIN32
WSAInitializer::WSAInitializer()
{
    WSADATA wsaData;
    // request versions in descending order
    for (const auto version : {Version::v2_2, Version::v2_1, Version::v2_0,
                               Version::v1_1, Version::v1_0}) {
        _error = WsaStartup(version, wsaData);
        if (0 == _error) {
            _selectedVersion = version;
            break;
        }
    }
}

WSAInitializer::~WSAInitializer()
{
    if (0 == error()) {
        ::WSACleanup();
    }
}

std::string WSAInitializer::ToString(Version version)
{
    // The version of the Windows Sockets specification that the Ws2_32.dll expects the caller to use.
    // The high-order byte specifies the minor version number;
    // the low-order byte specifies the major version number.
    return std::to_string(LOBYTE(version)) + "." + std::to_string(HIBYTE(version));
}

int WSAInitializer::WsaStartup(Version version, WSADATA& wsaData)
{
    int error = ::WSAStartup(static_cast<WORD>(version), &wsaData);
    if (0 == error && wsaData.wVersion != static_cast<WORD>(version)) {
        error = WSAVERNOTSUPPORTED;
        ::WSACleanup();
    }
    return error;
}
#endif

#endif

}
