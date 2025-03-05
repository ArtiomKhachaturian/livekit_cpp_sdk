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

namespace Websocket
{

enum class TlsMethod {
    /// Generic SSL version 2.
    sslv2,
    /// SSL version 2 client.
    sslv2_client,
    /// SSL version 2 server.
    sslv2_server,
    /// Generic SSL version 3.
    sslv3,
    /// SSL version 3 client.
    sslv3_client,
    /// SSL version 3 server.
    sslv3_server,
    /// Generic TLS version 1.
    tlsv1,
    /// TLS version 1 client.
    tlsv1_client,
    /// TLS version 1 server.
    tlsv1_server,
    /// Generic SSL/TLS.
    sslv23,
    /// SSL/TLS client.
    sslv23_client,
    /// SSL/TLS server.
    sslv23_server,
    /// Generic TLS version 1.1.
    tlsv11,
    /// TLS version 1.1 client.
    tlsv11_client,
    /// TLS version 1.1 server.
    tlsv11_server,
    /// Generic TLS version 1.2.
    tlsv12,
    /// TLS version 1.2 client.
    tlsv12_client,
    /// TLS version 1.2 server.
    tlsv12_server,
    /// Generic TLS version 1.3.
    tlsv13,
    /// TLS version 1.3 client.
    tlsv13_client,
    /// TLS version 1.3 server.
    tlsv13_server,
    /// Generic TLS.
    tls,
    /// TLS client.
    tls_client,
    /// TLS server.
    tls_server
};

} // namespace Websocket
