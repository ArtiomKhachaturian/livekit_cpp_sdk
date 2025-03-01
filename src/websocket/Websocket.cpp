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
#include "Websocket.h"
#include "WebsocketFailure.h"
#include <inttypes.h>
#include <sstream>

namespace {

std::string base64Encode(const uint8_t* data, size_t len);

inline std::string base64Encode(const std::string_view& str) {
    return base64Encode(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

}

namespace LiveKitCpp
{

WebsocketOptions::WebsocketOptions(std::string host, const std::string& user,
                                   const std::string& password)
    : _host(std::move(host))
{
    addAuthHeader(user, password);
}

WebsocketOptions::WebsocketOptions(const Url& url)
    : WebsocketOptions(url._host, url._user, url._password)
{
}

void WebsocketOptions::addAuthHeader(const std::string& user, const std::string& password)
{
    if (!user.empty() || !password.empty()) {
        // https://datatracker.ietf.org/doc/html/rfc6750.html
        auto auth = base64Encode(user + ":" + password);
        _extraHeaders["Authorization"] = "Basic " + auth;
    }
}

WebsocketError::WebsocketError(WebsocketFailure type, std::error_code code,
                               std::string_view details)
    : _type(type)
    , _code(std::move(code))
{
    if (!details.empty()) {
        _details.assign(details.data(), details.size());
    }
}

std::string WebsocketError::toString() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();
}

WebsocketError WebsocketError::fromSystemError(WebsocketFailure type,
                                               const std::system_error& error)
{
    return WebsocketError(type, error.code(), error.what());
}

const char* toString(WebsocketFailure failure) {
    switch (failure) {
        case WebsocketFailure::General:
            return "general";
        case WebsocketFailure::NoConnection:
            return "no connection";
        case WebsocketFailure::CustomHeader:
            return "custom header";
        case WebsocketFailure::WriteText:
            return "write text";
        case WebsocketFailure::WriteBinary:
            return "write binary";
        case WebsocketFailure::SocketOption:
            return "socket option";
        case WebsocketFailure::TlsOptions:
            return "TLS options";
        default:
            break;
    }
    return "unknown";
}

std::string toString(const WebsocketError& error)
{
    std::ostringstream stream;
    stream << error;
    return stream.str();
}

const char* toString(WebsocketState state) {
    switch (state) {
        case WebsocketState::Connecting:
            return "connecting";
        case WebsocketState::Connected:
            return "connected";
        case WebsocketState::Disconnecting:
            return "disconnecting";
        case WebsocketState::Disconnected:
            return "disconnected";
        default:
            break;
    }
    return "unknown";
}

} // namespace LiveKitCpp

std::ostream& operator << (std::ostream& os, const LiveKitCpp::WebsocketError& error)
{
    const auto& code = error.code();
    os << toString(error.type()) << " error";
    const auto message = code.message();
    if (!message.empty()) {
        os << " - '" << message << "'";
    }
    os << ", error code #" << code.value();
    os << ", category '" << code.category().name() << "'";
    const auto& details = error.details();
    if (!details.empty() && message != details) {
        os << ", details - '" << details << "'";
    }
    return os;
}

namespace {

constexpr size_t g_BufferOutSize = 65536U;
thread_local static uint8_t g_BufferOut[g_BufferOutSize];
const uint8_t g_Base64Table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const uint8_t* data, size_t len)
{
    const uint8_t* out = g_BufferOut;
    uint8_t* pos;
    const uint8_t* end;
    const uint8_t* in;
    size_t olen;

    olen = len * 4 / 3 + 4; // 3-byte blocks to 4-byte.

    if (olen < len) {
        // TODO: log error "integer overflow"
        return {};
    }
    if (olen > g_BufferOutSize - 1) {
        // TODO: log error "data too big"
        return {};
    }

    end = data + len;
    in  = data;
    pos = const_cast<uint8_t*>(out);

    while (end - in >= 3)
    {
        *pos++ = g_Base64Table[in[0] >> 2];
        *pos++ = g_Base64Table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = g_Base64Table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = g_Base64Table[in[2] & 0x3f];
        in += 3;
    }

    if (end - in)
    {
        *pos++ = g_Base64Table[in[0] >> 2];

        if (end - in == 1)
        {
            *pos++ = g_Base64Table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        }
        else
        {
            *pos++ = g_Base64Table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = g_Base64Table[(in[1] & 0x0f) << 2];
        }

        *pos++ = '=';
    }

    return { reinterpret_cast<const char*>(out), static_cast<size_t>(pos - out) };
}

}
