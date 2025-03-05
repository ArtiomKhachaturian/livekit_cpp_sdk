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
#include "WebsocketTlsPeerVerification.h"
#include "WebsocketTlsMethod.h"
#include <string>

namespace Websocket
{

struct Tls 
{
    TlsMethod _method = TlsMethod::sslv23;
    TlsPeerVerification _peerVerification = TlsPeerVerification::No;
    // PEM or ASN.1
    bool _certificateIsPem = true;
    // always create a new key when using _dh parameter
    bool _dhSingle = true;
    // disable SSL v2
    bool _sslv2No = true;
    // disable SSL v3
    bool _sslv3No = true;
    // disable TLS v1
    bool _tlsv1No = true;
    // disable TLS v1.1
    bool _tlsv1_1No = true;
    // disable TLS v1.2
    bool _tlsv1_2No = false;
    // disable TLS v1.3
    bool _tlsv1_3No = false;
    // don't use compression even if supported
    bool _sslNoCompression = false;
    std::string _certificate;
    std::string _certificatePrivateKey;
    std::string _certificatePrivateKeyPassword;
    std::string _trustStore;
    std::string _sslCiphers;
    std::string _dh; // Diffie-Hellman
};

} // namespace LiveKitCpp
