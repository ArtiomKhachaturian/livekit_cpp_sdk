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
#include "WebsocketFactory.h"
#include "LogsReceiver.h"
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
#include <system_wrappers/include/field_trial.h>
#include <libyuv/cpu_id.h>
#ifdef _WIN32
#include <optional>
#include <rtc_base/win32.h>
#endif
#endif

namespace {

#ifdef WEBRTC_AVAILABLE
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
    Impl(const std::shared_ptr<LogsReceiver>& logger,
         std::shared_ptr<WebsocketFactory> websocketsFactory);
    const auto& logger() const noexcept { return _logger; }
    const auto& websocketsFactory() const noexcept { return _websocketsFactory; }
    const auto& peerConnectionFactory() const noexcept { return _pcf; }
    static bool sslInitialized(const std::shared_ptr<LogsReceiver>& logger = {});
    static bool wsaInitialized(const std::shared_ptr<LogsReceiver>& logger = {});
    static std::unique_ptr<Impl> create(const std::shared_ptr<LogsReceiver>& logger,
                                        std::shared_ptr<WebsocketFactory> websocketsFactory);
private:
    static void testPlatform(const std::shared_ptr<LogsReceiver>& logger = {});
private:
    const std::shared_ptr<LogsReceiver> _logger;
    const std::shared_ptr<WebsocketFactory> _websocketsFactory;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
};

LiveKitService::LiveKitService(const std::shared_ptr<LogsReceiver>& logger,
                               const std::shared_ptr<WebsocketFactory>& websocketsFactory)
    : _impl(Impl::create(logger, websocketsFactory))
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

LiveKitService::Impl::Impl(const std::shared_ptr<LogsReceiver>& logger,
                           std::shared_ptr<WebsocketFactory> websocketsFactory)
    : _logger(logger)
    , _websocketsFactory(std::move(websocketsFactory))
    , _pcf(PeerConnectionFactory::Create(true, true, _logger))
{
}

bool LiveKitService::Impl::sslInitialized(const std::shared_ptr<LogsReceiver>& logger)
{
    static const SSLInitiallizer initializer;
    if (logger && !initializer) {
        logger->onError("Failed to SSL initialization");
    }
    return initializer.initialized();
}

bool LiveKitService::Impl::wsaInitialized(const std::shared_ptr<LogsReceiver>& logger)
{
#ifdef _WIN32
    static const WSAInitializer initializer;
    if (const auto error = initializer.GetError()) {
        if (logger) {
            logger->onError("Failed to WINSOCK initialization, error code: " + std::to_string(error));
        }
        return false;
    }
    if (logger) {
        const auto& wsaVersion = initializer.GetSelectedVersion();
        if (wsaVersion.has_value()) {
            logger->onVerbose("WINSOCK initialization is done, library version: " +
                              WSAInitializer::ToString(wsaVersion.value()));
        }
    }
#endif
    return true;
}

std::unique_ptr<LiveKitService::Impl> LiveKitService::Impl::
    create(const std::shared_ptr<LogsReceiver>& logger,
           std::shared_ptr<WebsocketFactory> websocketsFactory)
{
    if (wsaInitialized(logger) && sslInitialized(logger)) {
        if (!websocketsFactory) {
            websocketsFactory = WebsocketFactory::defaultFactory();
        }
        if (websocketsFactory) {
            testPlatform(logger);
            return std::make_unique<Impl>(logger, std::move(websocketsFactory));
        }
    }
    return {};
}

void LiveKitService::Impl::testPlatform(const std::shared_ptr<LogsReceiver>& logger)
{
    if (logger) {
#ifdef __APPLE__
        std::string envErrorInfo;
        const auto status = checkAppEnivonment(AESNoProblems, &envErrorInfo);
        if (AESNoProblems != status) {
            logger->onWarning("Some features of LiveKit SDK may be works incorrectly, see details below:");
            if (testFlag<AESNoGuiThread>(status)) {
                logger->onWarning(" - main thread is not available");
            }
            if (testFlag<AESNoInfoPlist>(status)) {
                logger->onWarning(" - application bundle info dictionary doesn't exist");
            }
            if (testFlag<AESIncompleteInfoPlist>(status)) {
                logger->onWarning(" - application bundle info dictionary is incomplete");
            }
            if (!envErrorInfo.empty()) {
                logger->onWarning(" - additional error info: " + envErrorInfo);
            }
        }
#endif
    }
}

#else
class LiveKitService::Impl {};

LiveKitService::LiveKitService(const std::shared_ptr<LogsReceiver>&,
                               const std::shared_ptr<WebsocketFactory>&) {}

LiveKitService::~LiveKitService() {}

LiveKitServiceState LiveKitService::state() const
{
    return LiveKitServiceState::NoWebRTC;
}

#endif

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
        //rtc::InitRandom(static_cast<int>(rtc::Time()));
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
