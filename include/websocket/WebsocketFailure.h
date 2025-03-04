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
#pragma once
#include "LiveKitClientExport.h"
#include <exception>
#include <ostream>
#include <string>
#include <system_error>

namespace LiveKitCpp
{

enum class WebsocketFailure
{
    General,
    NoConnection,
    CustomHeader,
    WriteText,
    WriteBinary,
    SocketOption,
    TlsOptions
};

class LIVEKIT_CLIENT_API WebsocketError
{
public:
    WebsocketError(WebsocketFailure type, std::error_code code,
                   std::string_view details = "");
    WebsocketFailure type() const noexcept { return _type; }
    const std::error_code& code() const noexcept { return _code; }
    const std::string& details() const noexcept { return _details; }
    std::string toString() const; // maybe for logs
    bool critical() const noexcept { return WebsocketFailure::SocketOption != type(); }
    static WebsocketError fromSystemError(WebsocketFailure type, const std::system_error& error);
private:
    WebsocketFailure _type;
    std::error_code _code;
    std::string _details;
};

LIVEKIT_CLIENT_API const char* toString(WebsocketFailure failure);
LIVEKIT_CLIENT_API std::string toString(const WebsocketError& error);

} // namespace LiveKitCpp

LIVEKIT_CLIENT_API std::ostream& operator << (std::ostream& os, const LiveKitCpp::WebsocketError& error);
