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
#ifdef USE_ZAPHOYD_TPP_SOCKETS
#include <websocketpp/common/asio.hpp>
#include <websocketpp/common/asio_ssl.hpp>
#include <websocketpp/common/system_error.hpp>

namespace LiveKitCpp
{

using WebsocketTppIOSrv = websocketpp::lib::asio::io_service;
using WebsocketTppSSLCtx = websocketpp::lib::asio::ssl::context;
using WebsocketTppSysError = websocketpp::lib::system_error;

} // namespace LiveKitCpp
#endif
