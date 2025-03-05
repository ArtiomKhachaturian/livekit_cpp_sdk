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
#include "WebsocketTpp.h"
#include "WebsocketListener.h"
#include "WebsocketFailure.h"
#include "WebsocketTppServiceProvider.h"
#include "WebsocketTppConfig.h"
#include "WebsocketTppApi.h"
#include "WebsocketTppExtensions.h"
#include "WebsocketTppMemoryBlock.h"
#include "Listeners.h"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/close.hpp>
#include <atomic>

namespace {

using namespace LiveKitCpp;

template<class TException, WebsocketFailure failure = WebsocketFailure::General>
inline WebsocketError makeError(const TException& e) {
    return WebsocketError(failure, e.code(), e.what());
}

class LogStream : public LoggableShared<std::streambuf>
{
public:
    LogStream(LoggingSeverity severity, const std::shared_ptr<LogsReceiver>& logger);
    ~LogStream() final;
    operator std::ostream* () { return &_output; }
    // overrides of std::streambuf
    std::streamsize xsputn(const char* s, std::streamsize count) final;
    int sync() final;
private:
    void sendBufferToLog();
private:
    const LoggingSeverity _severity;
    std::ostream _output;
    ProtectedObj<std::string, std::mutex> _logBuffer;
};

const std::string_view g_logCategory("WebsocketTPP");

}

namespace LiveKitCpp
{

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using namespace websocketpp::config;
using Hdl = websocketpp::connection_hdl;

template<class TClientType>
class WebsocketTpp::Impl : public WebsocketTppApi
{
    using Client = websocketpp::client<ReadBufferExtension<_readBufferSize, TClientType>>;
    using Message = typename TClientType::message_type;
    using MessagePtr = typename Message::ptr;
    using MessageMemoryBlock = WebsocketTppMemoryBlock<MessagePtr>;
public:
    ~Impl() override;
    // impl. of WebsocketTppApi
    bool open() final;
    std::string host() const final { return hostRef(); }
    State state() const final { return _state; }
    bool sendBinary(const std::shared_ptr<const MemoryBlock>& binary) final;
    bool sendText(const std::string_view& text) final;
    void destroy() final;
protected:
    Impl(uint64_t socketId, 
         uint64_t connectionId,
         WebsocketTppConfig config,
         const std::shared_ptr<WebsocketListener>& listener,
         const std::shared_ptr<WebsocketTppServiceProvider>& serviceProvider,
         const std::shared_ptr<LogsReceiver>& logger) noexcept(false);
    uint64_t socketId() const noexcept { return _socketId; }
    uint64_t connectionId() const noexcept { return _connectionId; }
    const auto& hostRef() const noexcept { return options()._host; }
    const auto& options() const noexcept { return _config.options(); }
    const auto& serviceProvider() const noexcept { return _serviceProvider; }
    const auto& client() const noexcept { return _client; }
    auto& client() noexcept { return _client; }
    void notifyAboutError(const WebsocketError& error);
    void notifyAboutError(WebsocketFailure type, const websocketpp::exception& error);
    void notifyAboutError(WebsocketFailure type, const WebsocketTppSysError& error);
    void notifyAboutError(WebsocketFailure type, std::error_code ec,
                          std::string_view details = "");
    template <class TOption>
    void setOption(const TOption& option, const Hdl& hdl);
    template <class TOption, typename T>
    void maybeSetOption(const std::optional<T>& value, const Hdl& hdl);
private:
    static std::string toText(MessagePtr message);
    static MessagePtr toMessage(const std::string_view& text);
    static MessagePtr toMessage(const std::shared_ptr<const MemoryBlock>& binary);
    static std::string formatLoggerId(uint64_t id);
    bool active() const noexcept { return !_destroyed; }
    // return true if state changed
    bool setState(State state);
    bool setState(websocketpp::session::state::value state);
    bool updateState();
    void setHdl(const Hdl& hdl);
    Hdl hdl() const;
    // handlers
    void onInit(const Hdl& hdl);
    void onFail(const Hdl& hdl);
    void onOpen(const Hdl& hdl);
    void onMessage(const Hdl& hdl, MessagePtr message);
    void onClose(const Hdl& hdl);
private:
    static constexpr auto _text = websocketpp::frame::opcode::value::text;
    static constexpr auto _binary = websocketpp::frame::opcode::value::binary;
    static constexpr uint16_t _closeCode = WebsocketCloseCode::Normal;
    const uint64_t _socketId;
    const uint64_t _connectionId;
    const WebsocketTppConfig _config;
    const std::shared_ptr<WebsocketListener> _listener;
    const std::shared_ptr<WebsocketTppServiceProvider> _serviceProvider;
    Client _client;
    LogStream _errorLogStream;
    ProtectedObj<Hdl> _hdl;
    std::atomic<State> _state = State::Disconnected;
    std::atomic_bool _destroyed = false;
};

class WebsocketTpp::TlsOn : public Impl<asio_tls_client>
{
public:
    TlsOn(uint64_t id, uint64_t connectionId,
          WebsocketTppConfig config,
          const std::shared_ptr<WebsocketListener>& listener,
          const std::shared_ptr<WebsocketTppServiceProvider>& serviceProvider,
          const std::shared_ptr<LogsReceiver>& logger) noexcept(false);
    ~TlsOn() final;
private:
    std::shared_ptr<WebsocketTppSSLCtx> onInitTls(const Hdl&);
};

class WebsocketTpp::TlsOff : public Impl<asio_client>
{
public:
    TlsOff(uint64_t id, uint64_t connectionId,
           WebsocketTppConfig config,
           const std::shared_ptr<WebsocketListener>& listener,
           const std::shared_ptr<WebsocketTppServiceProvider>& serviceProvider,
           const std::shared_ptr<LogsReceiver>& logger) noexcept(false);
};

class WebsocketTpp::Listener : public WebsocketListener
{
public:
    Listener() = default;
    void add(WebsocketListener* listener) { _listeners.add(listener); }
    void remove(WebsocketListener* listener) { _listeners.remove(listener); }
    // impl. of WebsocketListener
    void onStateChanged(uint64_t socketId, uint64_t connectionId,
                        const std::string_view& host, State state) final;
    void onError(uint64_t socketId, uint64_t connectionId,
                 const std::string_view& host, const WebsocketError& error) final;
    void onTextMessageReceived(uint64_t socketId, uint64_t connectionId,
                               const std::string_view& message) final;
    void onBinaryMessageReceved(uint64_t socketId, uint64_t connectionId,
                                const std::shared_ptr<const MemoryBlock>& message) final;
private:
    Listeners<WebsocketListener*> _listeners;
};

WebsocketTpp::WebsocketTpp(std::shared_ptr<WebsocketTppServiceProvider> serviceProvider,
                           const std::shared_ptr<LogsReceiver>& logger)
    : LoggableShared<Websocket>(logger)
    , _serviceProvider(std::move(serviceProvider))
    , _listener(std::make_shared<Listener>())
{
    assert(_serviceProvider); // service provider must not be null
}

WebsocketTpp::~WebsocketTpp()
{
    close();
}

void WebsocketTpp::addListener(WebsocketListener* listener)
{
    _listener->add(listener);
}

void WebsocketTpp::removeListener(WebsocketListener* listener)
{
    _listener->remove(listener);
}

bool WebsocketTpp::open(WebsocketOptions options, uint64_t connectionId)
{
    bool ok = false;
    LOCK_WRITE_PROTECTED_OBJ(_impl);
    if (!_impl.constRef() ||State::Disconnected == _impl.constRef()->state()) {
        auto impl = createImpl(std::move(options), connectionId);
        if (impl && impl->open()) {
            _impl = std::move(impl);
            ok = true;
        }
    }
    else { // connected or connecting now
        ok = true;
    }
    return ok;
}

void WebsocketTpp::close()
{
    std::shared_ptr<WebsocketTppApi> impl;
    {
        LOCK_WRITE_PROTECTED_OBJ(_impl);
        impl = _impl.take();
    }
    impl.reset();
}

std::string WebsocketTpp::host() const
{
    LOCK_READ_PROTECTED_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return impl->host();
    }
    return {};
}

State WebsocketTpp::state() const
{
    LOCK_READ_PROTECTED_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return impl->state();
    }
    return State::Disconnected;
}

bool WebsocketTpp::sendBinary(const std::shared_ptr<const MemoryBlock>& binary)
{
    LOCK_READ_PROTECTED_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return impl->sendBinary(binary);
    }
    return false;
}

bool WebsocketTpp::sendText(const std::string_view& text)
{
    LOCK_READ_PROTECTED_OBJ(_impl);
    if (const auto& impl = _impl.constRef()) {
        return impl->sendText(text);
    }
    return false;
}

std::shared_ptr<WebsocketTppApi> WebsocketTpp::createImpl(WebsocketOptions options,
                                                          uint64_t connectionId) const
{
    if (auto config = WebsocketTppConfig::create(std::move(options))) {
        // deep copy of host instance becase r-value references
        const std::string host(config.options()._host);
        try {
            std::unique_ptr<WebsocketTppApi> impl;
            if (config.secure()) {
                impl = std::make_unique<TlsOn>(id(), connectionId, std::move(config),
                                               _listener, _serviceProvider, logger());
            }
            else {
                impl = std::make_unique<TlsOff>(id(), connectionId, std::move(config),
                                                _listener, _serviceProvider, logger());
            }
            return std::shared_ptr<WebsocketTppApi>(impl.release(), [](auto* impl) { impl->destroy(); });
        }
        catch(const websocketpp::exception& e) {
            _listener->onError(id(), connectionId, host, makeError(e));
        }
        catch (const WebsocketTppSysError& e) {
            _listener->onError(id(), connectionId, host, makeError(e));
        }
    }
    return nullptr;
}

template <class TClientType>
WebsocketTpp::Impl<TClientType>::Impl(uint64_t socketId, uint64_t connectionId,
                                      WebsocketTppConfig config,
                                      const std::shared_ptr<WebsocketListener>& listener,
                                      const std::shared_ptr<WebsocketTppServiceProvider>& serviceProvider,
                                      const std::shared_ptr<LogsReceiver>& logger) noexcept(false)
    : _socketId(socketId)
    , _connectionId(connectionId)
    , _config(std::move(config))
    , _listener(listener)
    , _serviceProvider(serviceProvider)
    , _errorLogStream(LoggingSeverity::Error, logger)
{
    // Initialize ASIO
    _client.set_user_agent(options()._userAgent);
    _client.get_alog().set_channels(websocketpp::log::alevel::none);
    _client.get_elog().set_ostream(_errorLogStream);
    _client.init_asio(_serviceProvider->service());
    // Register our handlers
    _client.set_socket_init_handler(bind(&Impl::onInit, this, _1));
    _client.set_message_handler(bind(&Impl::onMessage, this, _1, _2));
    _client.set_open_handler(bind(&Impl::onOpen, this, _1));
    _client.set_close_handler(bind(&Impl::onClose, this, _1));
    _client.set_fail_handler(bind(&Impl::onFail, this, _1));
    _client.start_perpetual();
    _serviceProvider->startService();
    
}

template <class TClientType>
WebsocketTpp::Impl<TClientType>::~Impl()
{
    _client.set_socket_init_handler(nullptr);
    _client.set_message_handler(nullptr);
    _client.set_open_handler(nullptr);
    _client.set_fail_handler(nullptr);
    _client.set_close_handler(nullptr);
    _client.stop_perpetual();
    _serviceProvider->stopService();
}

template <class TClientType>
bool WebsocketTpp::Impl<TClientType>::open()
{
    websocketpp::lib::error_code ec;
    const auto connection = _client.get_connection(_config, ec);
    if (ec) {
        notifyAboutError(WebsocketFailure::NoConnection, ec);
    }
    else {
        const auto& extraHeaders = options()._extraHeaders;
        for (auto it = extraHeaders.begin(); it != extraHeaders.end(); ++it) {
            try {
                connection->append_header(it->first, it->second);
            }
            catch(const websocketpp::exception& e) {
                notifyAboutError(WebsocketFailure::CustomHeader, e);
                return false;
            }
            catch (const WebsocketTppSysError& e) {
                notifyAboutError(WebsocketFailure::CustomHeader, e);
                return false;
            }
        }
        _client.connect(connection);
    }
    return !ec;
}

template <class TClientType>
bool WebsocketTpp::Impl<TClientType>::sendText(const std::string_view& text)
{
    if (active()) {
        try {
            _client.send(hdl(), toMessage(text));
            return true;
        }
        catch (const websocketpp::exception& e) {
            notifyAboutError(WebsocketFailure::WriteText, e);
        }
        catch (const WebsocketTppSysError& e) {
            notifyAboutError(WebsocketFailure::WriteText, e);
        }
    }
    return false;
}

template <class TClientType>
bool WebsocketTpp::Impl<TClientType>::sendBinary(const std::shared_ptr<const MemoryBlock>& binary)
{
    if (active()) {
        try {
            _client.send(hdl(), toMessage(binary));
            return true;
        }
        catch (const websocketpp::exception& e) {
            notifyAboutError(WebsocketFailure::WriteBinary, e);
        }
        catch (const WebsocketTppSysError& e) {
            notifyAboutError(WebsocketFailure::WriteBinary, e);
        }
    }
    return false;
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::destroy()
{
    if (!_destroyed.exchange(true)) {
        setState(State::Disconnecting);
        {
            LOCK_WRITE_PROTECTED_OBJ(_hdl);
            auto hdl = _hdl.take();
            if (!hdl.expired()) {
                try {
                    const auto reason = websocketpp::close::status::get_string(_closeCode);
                    // instance will be destroyed in [OnClose] handler
                    _client.close(hdl, _closeCode, reason);
                    setState(State::Disconnected); // force
                    return;
                }
                catch (const std::exception& e) {
                    // ignore of failures during closing
                    _client.set_close_handler(nullptr);
                }
            }
        }
        setState(State::Disconnected);
        delete this;
    }
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::notifyAboutError(const WebsocketError& error)
{
    if (_errorLogStream.canLogError()) {
        _errorLogStream.logError(toString(error), g_logCategory);
    }
    _listener->onError(socketId(), connectionId(), hostRef(), error);
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::notifyAboutError(WebsocketFailure type,
                                                       const websocketpp::exception& error)
{
    notifyAboutError(type, error.code(), error.what());
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::notifyAboutError(WebsocketFailure type,
                                                       const WebsocketTppSysError& error)
{
    notifyAboutError(type, error.code(), error.what());
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::notifyAboutError(WebsocketFailure type,
                                                       std::error_code ec,
                                                       std::string_view details)
{
    notifyAboutError(WebsocketError(type, std::move(ec), std::move(details)));
}

template <class TClientType>
template <class TOption>
void WebsocketTpp::Impl<TClientType>::setOption(const TOption& option, const Hdl& hdl)
{
    if (!hdl.expired()) {
        try {
            if (const auto connection = _client.get_con_from_hdl(hdl)) {
                connection->get_socket().lowest_layer().set_option(option);
            }
        }
        catch (const websocketpp::exception& e) {
            notifyAboutError(WebsocketFailure::SocketOption, e);
        }
        catch (const WebsocketTppSysError& e) {
            notifyAboutError(WebsocketFailure::SocketOption, e);
        }
    }
}

template <class TClientType>
template <class TOption, typename T>
void WebsocketTpp::Impl<TClientType>::maybeSetOption(const std::optional<T>& value,
                                                     const Hdl& hdl)
{
    if (value) {
        setOption(ValueToOption<TOption, T>::convert(value.value()), hdl);
    }
}

template <class TClientType>
std::string WebsocketTpp::Impl<TClientType>::toText(MessagePtr message)
{
    if (message) {
        auto text = std::move(message->get_raw_payload());
        message->recycle();
        return text;
    }
    return std::string();
}

template <class TClientType>
typename WebsocketTpp::Impl<TClientType>::MessagePtr
    WebsocketTpp::Impl<TClientType>::toMessage(const std::string_view& text)
{
    auto message = std::make_shared<Message>(nullptr, _text, 0U);
    if (!text.empty()) {
        message->get_raw_payload() = std::string(text.data(), text.size());
    }
    return message;
}

template <class TClientType>
typename WebsocketTpp::Impl<TClientType>::MessagePtr
    WebsocketTpp::Impl<TClientType>::toMessage(const std::shared_ptr<const MemoryBlock>& binary)
{
    auto message = std::make_shared<Message>(nullptr, _binary, 0U);
    if (binary) {
        // overhead - deep copy of input buffer,
        // Websocketpp doesn't supports of buffers abstraction,
        // workarounds with custom allocator for payload std::string doesn't works:
        //  - allocation size which is passed to [allocate] method is
        //    always greater than buffer capacity (penalty of std::string internals)
        //  - resize operation (until C++23) erases buffer data,
        //    at least we need [resize_and_overwrite]: https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
        // TODO: possible solution -> make a fork of Websocket TPP repo and replace std::string to buffer abstraction
        auto& payload = message->get_raw_payload();
        const auto data = reinterpret_cast<const char*>(binary->data());
        payload.assign(data, binary->size());
    }
    return message;
}


template <class TClientType>
std::string WebsocketTpp::Impl<TClientType>::formatLoggerId(uint64_t id)
{
    return "WebsocketTpp (id #" + std::to_string(id) + ")";
}

template <class TClientType>
bool WebsocketTpp::Impl<TClientType>::setState(State state)
{
    if (state != _state.exchange(state)) {
        _listener->onStateChanged(socketId(), connectionId(), hostRef(), state);
        return true;
    }
    return false;
}

template <class TClientType>
bool WebsocketTpp::Impl<TClientType>::setState(websocketpp::session::state::value state)
{
    switch (state) {
        case websocketpp::session::state::connecting:
            return setState(State::Connecting);
        case websocketpp::session::state::open:
            return setState(State::Connected);
        case websocketpp::session::state::closing:
            return setState(State::Disconnecting);
        case websocketpp::session::state::closed:
            return setState(State::Disconnected);
        default:
            assert(false); // unknown state
            break;
    }
    return false;
}

template <class TClientType>
bool WebsocketTpp::Impl<TClientType>::updateState()
{
    websocketpp::lib::error_code ec; // for supression of exception
    if (const auto conn = _client.get_con_from_hdl(hdl(), ec)) {
        return setState(conn->get_state());
    }
    return setState(State::Disconnected);
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::setHdl(const Hdl& hdl)
{
    LOCK_WRITE_PROTECTED_OBJ(_hdl);
    _hdl = hdl;
}

template <class TClientType>
Hdl WebsocketTpp::Impl<TClientType>::hdl() const
{
    LOCK_READ_PROTECTED_OBJ(_hdl);
    return _hdl.constRef();
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::onInit(const Hdl& hdl)
{
    setHdl(hdl);
    updateState();
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::onFail(const Hdl& hdl)
{
    websocketpp::lib::error_code ec;
    if (const auto connection = _client.get_con_from_hdl(hdl, ec)) {
        ec = connection->get_ec();
    }
    if (ec) {
        // report error
        notifyAboutError(WebsocketFailure::General, ec);
    }
    updateState();
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::onOpen(const Hdl& hdl)
{
    maybeSetOption<asio::socket_base::broadcast>(options()._broadcast, hdl);
    maybeSetOption<asio::socket_base::do_not_route>(options()._doNotRoute, hdl);
    maybeSetOption<asio::socket_base::keep_alive>(options()._keepAlive, hdl);
    maybeSetOption<asio::socket_base::linger>(options()._linger, hdl);
    maybeSetOption<asio::socket_base::receive_buffer_size>(options()._receiveBufferSize, hdl);
    maybeSetOption<asio::socket_base::receive_low_watermark>(options()._receiveLowWatermark, hdl);
    maybeSetOption<asio::socket_base::reuse_address>(options()._reuseAddress, hdl);
    maybeSetOption<asio::socket_base::send_buffer_size>(options()._sendBufferSize, hdl);
    maybeSetOption<asio::socket_base::send_low_watermark>(options()._sendLowWatermark, hdl);
    maybeSetOption<asio::ip::tcp::no_delay>(options()._tcpNoDelay, hdl);
    updateState();
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::onMessage(const Hdl& hdl, MessagePtr message)
{
    if (message) {
        switch (message->get_opcode()) {
            case _text:
                _listener->onTextMessageReceived(socketId(), connectionId(),
                                                 toText(std::move(message)));
                break;
            case _binary:
                _listener->onBinaryMessageReceved(socketId(), connectionId(),
                                                  std::make_shared<MessageMemoryBlock>(std::move(message)));
                break;
            default:
                break;
        }
    }
}

template <class TClientType>
void WebsocketTpp::Impl<TClientType>::onClose(const Hdl&)
{
    if (_destroyed) {
        delete this;
    }
    else {
        updateState();
        setHdl({});
    }
}

WebsocketTpp::TlsOn::TlsOn(uint64_t id, uint64_t connectionId,
                           WebsocketTppConfig config,
                           const std::shared_ptr<WebsocketListener>& listener,
                           const std::shared_ptr<WebsocketTppServiceProvider>& serviceProvider,
                           const std::shared_ptr<LogsReceiver>& logger) noexcept(false)
    : Impl<asio_tls_client>(id, connectionId, std::move(config), listener, serviceProvider, logger)
{
    client().set_tls_init_handler(bind(&TlsOn::onInitTls, this, _1));
}

WebsocketTpp::TlsOn::~TlsOn()
{
    client().set_tls_init_handler(nullptr);
}

std::shared_ptr<WebsocketTppSSLCtx> WebsocketTpp::TlsOn::onInitTls(const Hdl&)
{
    try {
        return serviceProvider()->createSSLContext(options()._tls);
    } catch (const std::system_error& e) {
        notifyAboutError(WebsocketFailure::TlsOptions, e);
    }
    return nullptr;
}

WebsocketTpp::TlsOff::TlsOff(uint64_t id, uint64_t connectionId,
                             WebsocketTppConfig config,
                             const std::shared_ptr<WebsocketListener>& listener,
                             const std::shared_ptr<WebsocketTppServiceProvider>& serviceProvider,
                             const std::shared_ptr<LogsReceiver>& logger) noexcept(false)
    : Impl<asio_client>(id, connectionId, std::move(config), listener, serviceProvider, logger)
{
}

void WebsocketTpp::Listener::onStateChanged(uint64_t socketId,
                                            uint64_t connectionId,
                                            const std::string_view& host,
                                            State state)
{
    WebsocketListener::onStateChanged(socketId, connectionId, host, state);
    _listeners.invokeMethod(&WebsocketListener::onStateChanged, socketId,
                            connectionId, host, state);
}

void WebsocketTpp::Listener::onError(uint64_t socketId,
                                     uint64_t connectionId,
                                     const std::string_view& host,
                                     const WebsocketError& error)
{
    WebsocketListener::onError(socketId, connectionId, host, error);
    _listeners.invokeMethod(&WebsocketListener::onError, socketId,
                            connectionId, host, error);
}

void WebsocketTpp::Listener::onTextMessageReceived(uint64_t socketId,
                                                   uint64_t connectionId,
                                                   const std::string_view& message)
{
    WebsocketListener::onTextMessageReceived(socketId, connectionId, message);
    _listeners.invokeMethod(&WebsocketListener::onTextMessageReceived,
                            socketId, connectionId, message);
}

void WebsocketTpp::Listener::onBinaryMessageReceved(uint64_t socketId,
                                                    uint64_t connectionId,
                                                    const std::shared_ptr<const MemoryBlock>& message)
{
    WebsocketListener::onBinaryMessageReceved(socketId, connectionId, message);
    _listeners.invokeMethod(&WebsocketListener::onBinaryMessageReceved,
                            socketId, connectionId, message);
}

} // namespace LiveKitCpp

namespace {

LogStream::LogStream(LoggingSeverity severity, const std::shared_ptr<LogsReceiver>& logger)
    : LoggableShared<std::streambuf>(logger)
    , _severity(severity)
    , _output(this)
{
}

std::streamsize LogStream::xsputn(const char* s, std::streamsize count)
{
    if (s && count && canLog(_severity)) {
        std::string_view data(s, count);
        if (data.front() == '\n') {
            data = data.substr(1U, data.size() - 1U);
        }
        if (data.back() == '\n') {
            data = data.substr(0U, data.size() - 1U);
        }
        if (!data.empty()) {
            LOCK_WRITE_PROTECTED_OBJ(_logBuffer);
            _logBuffer->append(data.data(), data.size());
        }
    }
    return count;
}

int LogStream::sync()
{
    sendBufferToLog();
    return 0;
}

LogStream::~LogStream()
{
    if (pbase() != pptr()) {
        sendBufferToLog();
    }
}

void LogStream::sendBufferToLog()
{
    LOCK_WRITE_PROTECTED_OBJ(_logBuffer);
    if (!_logBuffer->empty()) {
        log(_severity, _logBuffer.constRef(), g_logCategory);
        _logBuffer->clear();
    }
}

}

#endif
