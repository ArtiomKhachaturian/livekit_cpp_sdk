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
#include "LiveKitClientExport.h"
#include "e2e/KeyProvider.h"
#include "e2e/KeyProviderOptions.h"

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class LIVEKIT_CLIENT_API DefaultKeyProvider : public KeyProvider
{
    struct Impl;
public:
    DefaultKeyProvider(KeyProviderOptions options = {},
                       const std::shared_ptr<Bricks::Logger>& logger = {});
    ~DefaultKeyProvider() final;
    // impl. of KeyProvider
    bool setSharedKey(std::vector<uint8_t> key, const std::optional<uint8_t>& keyIndex = {}) final;
    std::shared_ptr<ParticipantKeyHandler> sharedKey(const std::string& participantSid) final;
    std::vector<uint8_t> ratchetSharedKey(const std::optional<uint8_t>& keyIndex = {}) final;
    std::vector<uint8_t> exportSharedKey(const std::optional<uint8_t>& keyIndex = {}) const final;
    bool setKey(const std::string& participantSid, std::vector<uint8_t> key,
                const std::optional<uint8_t>& keyIndex = {}) final;
    std::shared_ptr<ParticipantKeyHandler> key(const std::string& participantSid) const final;
    std::vector<uint8_t> ratchetKey(const std::string& participantSid,
                                    const std::optional<uint8_t>& keyIndex = {}) const final;
    std::vector<uint8_t> exportKey(const std::string& participantSid,
                                   const std::optional<uint8_t>& keyIndex = {}) const final;
    void setSifTrailer(std::vector<uint8_t> trailer) final;
    std::vector<uint8_t> sifTrailer() const final;
    const KeyProviderOptions& options() const final;
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
