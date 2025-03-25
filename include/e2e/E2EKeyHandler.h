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
#pragma once // ParticipantKeyHandler.h
#include "LiveKitClientExport.h"
#include "e2e/KeySet.h"
#include <memory>
#include <optional>
#include <string_view>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

struct KeyProviderOptions;

class LIVEKIT_CLIENT_API E2EKeyHandler
{
    struct Impl;
public:
    E2EKeyHandler(const KeyProviderOptions& options,
                  const std::shared_ptr<Bricks::Logger>& logger = {});
    virtual ~E2EKeyHandler();
    std::shared_ptr<E2EKeyHandler> clone() const;
    virtual std::vector<uint8_t> ratchetKey(const std::optional<uint8_t>& keyIndex = {});
    virtual std::shared_ptr<KeySet> keySet(const std::optional<uint8_t>& keyIndex = {}) const;
    virtual void setKey(std::vector<uint8_t> password,
                        const std::optional<uint8_t>& keyIndex = {});
    void setKey(std::string_view password,
                const std::optional<uint8_t>& keyIndex = {});
    std::vector<uint8_t> ratchetKeyMaterial(const std::vector<uint8_t>& currentMaterial) const;
    std::shared_ptr<KeySet> deriveKeys(const std::vector<uint8_t>& ratchetSalt,
                                       std::vector<uint8_t> password,
                                       unsigned int optionalLengthBits) const;
    std::shared_ptr<KeySet> deriveKeys(const std::vector<uint8_t>& ratchetSalt,
                                       std::string_view password,
                                       unsigned int optionalLengthBits) const;
    bool hasValidKey() const;
    void setHasValidKey();
    void setKeyFromMaterial(std::vector<uint8_t> password,
                            const std::optional<uint8_t>& keyIndex = {});
    void setKeyFromMaterial(std::string_view password,
                            const std::optional<uint8_t>& keyIndex = {});
    bool decryptionFailure();
private:
    E2EKeyHandler(std::unique_ptr<Impl> impl);
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
