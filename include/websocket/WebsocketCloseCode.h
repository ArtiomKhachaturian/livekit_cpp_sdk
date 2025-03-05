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
#include <cstdint>
    
namespace Websocket
{

// https://github.com/Luka967/websocket-close-codes
namespace CloseCode
{
    static const uint16_t Normal                = 1000;
    static const uint16_t GoingAway             = 1001;
    static const uint16_t ProtocolError         = 1002;
    static const uint16_t UnsupportedData       = 1003;
    static const uint16_t NoStatus              = 1005;
    static const uint16_t AbnormalClose         = 1006;
    static const uint16_t InvalidPayload        = 1007;
    static const uint16_t PolicyViolation       = 1008;
    static const uint16_t MessageTooBig         = 1009;
    static const uint16_t ExtensionRequired     = 1010;
    static const uint16_t InternalEndpointError = 1011;
    static const uint16_t serviceRestart        = 1012;
    static const uint16_t TryAgainLater         = 1013;
    static const uint16_t BadGateway            = 1014;
    static const uint16_t TlsHandshake          = 1015;
};

} // namespace Websocket
