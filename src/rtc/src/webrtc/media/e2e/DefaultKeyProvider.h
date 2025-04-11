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
#pragma once // DefaultKeyProvider.h
#include "Loggable.h"
#include "SafeObj.h"
#include "livekit/rtc/e2e/KeyProvider.h"
#include "livekit/rtc/e2e/KeyProviderOptions.h"
#include <unordered_map>


namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class DefaultKeyProvider : public Bricks::LoggableS<KeyProvider>
{
    using ParticipantKeyHandlers = std::unordered_map<std::string, std::shared_ptr<E2EKeyHandler>>;
public:
    DefaultKeyProvider(KeyProviderOptions options = {},
                       const std::shared_ptr<Bricks::Logger>& logger = {});
    ~DefaultKeyProvider() final;
    // impl. of KeyProvider
    bool setSharedKey(std::vector<uint8_t> key, const std::optional<uint8_t>& keyIndex = {}) final;
    std::shared_ptr<E2EKeyHandler> sharedKey(const std::string& identity) final;
    std::vector<uint8_t> ratchetSharedKey(const std::optional<uint8_t>& keyIndex = {}) final;
    std::vector<uint8_t> exportSharedKey(const std::optional<uint8_t>& keyIndex = {}) const final;
    bool setKey(const std::string& identity, std::vector<uint8_t> key,
                const std::optional<uint8_t>& keyIndex = {}) final;
    std::shared_ptr<E2EKeyHandler> key(const std::string& identity) const final;
    std::vector<uint8_t> ratchetKey(const std::string& identity,
                                    const std::optional<uint8_t>& keyIndex = {}) const final;
    std::vector<uint8_t> exportKey(const std::string& identity,
                                   const std::optional<uint8_t>& keyIndex = {}) const final;
    void setSifTrailer(std::vector<uint8_t> trailer) final { _sifTrailer(std::move(trailer)); }
    std::vector<uint8_t> sifTrailer() const final { return _sifTrailer(); }
    const KeyProviderOptions& options() const final { return _options; }
private:
    std::shared_ptr<E2EKeyHandler> newKeyHandler() const;
    std::shared_ptr<E2EKeyHandler> emplaceHandler(const std::string& identity);
private:
    static inline const std::string _shared = "shared";
    const KeyProviderOptions _options;
    Bricks::SafeObj<ParticipantKeyHandlers> _keys;
    Bricks::SafeObj<std::vector<uint8_t>> _sifTrailer; // sif
};

} // namespace LiveKitCpp
