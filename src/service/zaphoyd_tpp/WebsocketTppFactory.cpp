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
#ifdef USE_ZAPHOYD_TPP_SOCKETS
#include "WebsocketTppFactory.h"
#include "WebsocketTpp.h"
#include "WebsocketTppServiceProvider.h"
#include "WebsocketTls.h"
#include "ThreadExecution.h"
#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
#include "ProtectedObj.h"
#endif
#include <absl/memory/memory.h>
#include <optional>

namespace {

inline auto makeBuffer(const std::string& buffer) {
    return websocketpp::lib::asio::const_buffer(buffer.data(), buffer.size());
}

inline auto format(bool pem) {
    if (pem) {
        return LiveKitCpp::WebsocketTppSSLCtx::file_format::pem;
    }
    return LiveKitCpp::WebsocketTppSSLCtx::file_format::asn1;
}

}

namespace LiveKitCpp
{

class WebsocketTppFactory::ServiceProvider : public WebsocketTppServiceProvider,
                                             private ThreadExecution
{
public:
    ServiceProvider();
    ~ServiceProvider() final { stopExecution(); }
    // impl. of WebsocketTppServiceProvider
#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
    void startService() final;
    void stopService() final;
#else
    void startService() final { startExecution(); }
    void stopService() final { stopExecution(); }
#endif
    WebsocketTppIOSrv* service() final { return _service.get(); }
    std::shared_ptr<WebsocketTppSSLCtx> createSSLContext(const WebsocketTls& tls) const final;
protected:
    // impl. of ThreadExecution
    void doExecuteInThread() final;
    void doStopThread() final;
private:
    static std::optional<WebsocketTppSSLCtx::method> convert(WebsocketTlsMethod method);
    static const auto& sslErrorCategory() { return websocketpp::lib::asio::error::get_ssl_category(); }
private:
    const std::shared_ptr<WebsocketTppIOSrv> _service;
#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
    ProtectedObj<uint64_t, std::mutex> _counter = 0U;
#endif
};

WebsocketTppFactory::WebsocketTppFactory()
#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
    : _serviceProvider(std::make_shared<ServiceProvider>())
#endif
{
}

WebsocketTppFactory::~WebsocketTppFactory()
{
}

std::shared_ptr<WebsocketTppServiceProvider> WebsocketTppFactory::serviceProvider() const
{
#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
    return _serviceProvider;
#else
    return std::make_shared<ServiceProvider>();
#endif
}

std::unique_ptr<Websocket> WebsocketTppFactory::create() const
{
    return std::make_unique<WebsocketTpp>(serviceProvider());
}

WebsocketTppFactory::ServiceProvider::ServiceProvider()
    : ThreadExecution("WebsocketsTPP", ThreadPriority::Highest)
    , _service(std::make_shared<WebsocketTppIOSrv>())
{
}

#ifdef WEBSOCKETS_TPP_SHARED_IO_SERVICE
void WebsocketTppFactory::ServiceProvider::startService()
{
    LOCK_WRITE_PROTECTED_OBJ(_counter);
    if (0U == _counter.ref()++) {
        startExecution();
    }
}

void WebsocketTppFactory::ServiceProvider::stopService()
{
    LOCK_WRITE_PROTECTED_OBJ(_counter);
    if (_counter.constRef() && 1U == _counter.ref()--) {
        stopExecution();
    }
}
#endif

std::shared_ptr<WebsocketTppSSLCtx> WebsocketTppFactory::ServiceProvider::
    createSSLContext(const WebsocketTls& tls) const
{
    if (const auto method = convert(tls._method)) {
        try {
            auto ctx = std::make_shared<WebsocketTppSSLCtx>(method.value());
            bool verify = false;
            if (!tls._trustStore.empty()) {
                ctx->add_certificate_authority(makeBuffer(tls._trustStore));
                verify = true;
            }
            if (!tls._certificate.empty()) {
                ctx->use_certificate_chain(makeBuffer(tls._certificate));
                ctx->use_private_key(makeBuffer(tls._certificatePrivateKey),
                                     format(tls._certificateIsPem));
                auto callback = [password = tls._certificatePrivateKeyPassword](
                    std::size_t /*size*/, WebsocketTppSSLCtx::password_purpose /*purpose*/) {
                        return password;
                };
                ctx->set_password_callback(std::move(callback));
                verify = true;
            }
            if (!tls._sslCiphers.empty()) {
                const auto res = SSL_CTX_set_cipher_list(ctx->native_handle(), tls._sslCiphers.c_str());
                if (0 == res) { // https://www.openssl.org/docs/man3.1/man3/SSL_CTX_set_cipher_list.html
                    // return 1 if any cipher could be selected and 0 on complete failure
                    const auto error = static_cast<int>(ERR_get_error());
                    throw std::system_error(error, sslErrorCategory(), "SSL_CTX_set_cipher_list");
                }
            }
            if (!tls._dh.empty()) {
                const auto& dh = tls._dh;
                ctx->use_tmp_dh(makeBuffer(dh));
            }
            if (verify) {
                const auto& verification = tls._peerVerification;
                if (WebsocketTlsPeerVerification::No != verification) {
                    WebsocketTppSSLCtx::verify_mode mode = WebsocketTppSSLCtx::verify_peer;
                    if (WebsocketTlsPeerVerification::YesAndRejectIfNoCert == verification) {
                        mode |= WebsocketTppSSLCtx::verify_fail_if_no_peer_cert;
                    }
                    ctx->set_verify_mode(mode);
                }
            }
            WebsocketTppSSLCtx::options options = WebsocketTppSSLCtx::default_workarounds;
            if (tls._dhSingle) {
                options |= WebsocketTppSSLCtx::single_dh_use;
            }
            if (tls._sslv2No) {
                options |= WebsocketTppSSLCtx::no_sslv2;
            }
            if (tls._sslv3No) {
                options |= WebsocketTppSSLCtx::no_sslv3;
            }
            if (tls._tlsv1No) {
                options |= WebsocketTppSSLCtx::no_tlsv1;
            }
            if (tls._tlsv1_1No) {
                options |= WebsocketTppSSLCtx::no_tlsv1_1;
            }
            if (tls._tlsv1_2No) {
                options |= WebsocketTppSSLCtx::no_tlsv1_2;
            }
            if (tls._tlsv1_3No) {
                options |= WebsocketTppSSLCtx::no_tlsv1_3;
            }
            if (tls._sslNoCompression) {
                options |= WebsocketTppSSLCtx::no_compression;
            }
            ctx->set_options(options);
            return ctx;
        }
        catch (const WebsocketTppSysError& e) {
            const auto& code = e.code();
            throw std::system_error(code.value(), code.category());
        }
    }
    return nullptr;
}

void WebsocketTppFactory::ServiceProvider::doExecuteInThread()
{
    // local copy for keep lifetime if thread was detached
    const auto service(_service);
    websocketpp::lib::asio::error_code ec;
    service->run(ec);
    if (ec) {
        // TODO: add ASIO error logs
    }
}

void WebsocketTppFactory::ServiceProvider::doStopThread()
{
    _service->stop();
}

std::optional<WebsocketTppSSLCtx::method> WebsocketTppFactory::ServiceProvider::
    convert(WebsocketTlsMethod method)
{
    switch (method) {
        case WebsocketTlsMethod::sslv2:
            return WebsocketTppSSLCtx::method::sslv2;
        case WebsocketTlsMethod::sslv2_client:
            return WebsocketTppSSLCtx::method::sslv2_client;
        case WebsocketTlsMethod::sslv2_server:
            return WebsocketTppSSLCtx::method::sslv2_server;
        case WebsocketTlsMethod::sslv3:
            return WebsocketTppSSLCtx::method::sslv3;
        case WebsocketTlsMethod::sslv3_client:
            return WebsocketTppSSLCtx::method::sslv3_client;
        case WebsocketTlsMethod::sslv3_server:
            return WebsocketTppSSLCtx::method::sslv3_server;
        case WebsocketTlsMethod::tlsv1:
            return WebsocketTppSSLCtx::method::tlsv1;
        case WebsocketTlsMethod::tlsv1_client:
            return WebsocketTppSSLCtx::method::tlsv1_client;
        case WebsocketTlsMethod::tlsv1_server:
            return WebsocketTppSSLCtx::method::tlsv1_server;
        case WebsocketTlsMethod::sslv23:
            return WebsocketTppSSLCtx::method::sslv23;
        case WebsocketTlsMethod::sslv23_client:
            return WebsocketTppSSLCtx::method::sslv23_client;
        case WebsocketTlsMethod::sslv23_server:
            return WebsocketTppSSLCtx::method::sslv23_server;
        case WebsocketTlsMethod::tlsv11:
            return WebsocketTppSSLCtx::method::tlsv11;
        case WebsocketTlsMethod::tlsv11_client:
            return WebsocketTppSSLCtx::method::tlsv11_client;
        case WebsocketTlsMethod::tlsv11_server:
            return WebsocketTppSSLCtx::method::tlsv11_server;
        case WebsocketTlsMethod::tlsv12:
            return WebsocketTppSSLCtx::method::tlsv12;
        case WebsocketTlsMethod::tlsv12_client:
            return WebsocketTppSSLCtx::method::tlsv12_client;
        case WebsocketTlsMethod::tlsv12_server:
            return WebsocketTppSSLCtx::method::tlsv12_server;
        case WebsocketTlsMethod::tlsv13:
            return WebsocketTppSSLCtx::method::tlsv13;
        case WebsocketTlsMethod::tlsv13_client:
            return WebsocketTppSSLCtx::method::tlsv13_client;
        case WebsocketTlsMethod::tlsv13_server:
            return WebsocketTppSSLCtx::method::tlsv13_server;
        case WebsocketTlsMethod::tls:
            return WebsocketTppSSLCtx::method::tls;
        case WebsocketTlsMethod::tls_client:
            return WebsocketTppSSLCtx::method::tls_client;
        case WebsocketTlsMethod::tls_server:
            return WebsocketTppSSLCtx::method::tls_server;
        default:
            break;
    }
    return std::nullopt;
}

} // namespace LiveKitCpp
#endif
