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
#pragma once // KeyProviderOptions.h
#include "livekit/LiveKitClientExport.h"
#include <optional>
#include <string_view>
#include <vector>

namespace LiveKitCpp
{

// https://github.com/livekit/client-sdk-js/blob/main/src/e2ee/constants.ts#L34
struct KeyProviderOptions
{
    bool _sharedKey = true;
    std::vector<uint8_t> _ratchetSalt;
    size_t _ratchetWindowSize = 0U;
    std::optional<uint64_t> _failureTolerance;
    // key ring size should be between 1 and 255
    uint8_t _keyRingSize = _defaultKeyRingSize;
    bool _discardFrameWhenCryptorNotReady = false;
    LIVEKIT_CLIENT_API void setRatchetSalt(std::string_view salt);
    // constants
    static inline constexpr uint8_t _defaultKeyRingSize = 16;
    static inline constexpr uint8_t _maxKeyRingSize = 255;
};

} // namespace LiveKitCpp
