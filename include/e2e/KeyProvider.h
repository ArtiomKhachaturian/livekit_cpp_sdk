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
#pragma once // KeyProvider.h
#include "LiveKitClientExport.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace LiveKitCpp
{

class E2EKeyHandler;
struct KeyProviderOptions;

// [identity] is a participant identity (Participant::identity)
class LIVEKIT_CLIENT_API KeyProvider
{
public:
    virtual ~KeyProvider() = default;
    virtual bool setSharedKey(std::vector<uint8_t> key,
                              const std::optional<uint8_t>& keyIndex = {}) = 0;
    virtual std::shared_ptr<E2EKeyHandler> sharedKey(const std::string& identity) = 0;
    virtual std::vector<uint8_t> ratchetSharedKey(const std::optional<uint8_t>& keyIndex = {}) = 0;
    virtual std::vector<uint8_t> exportSharedKey(const std::optional<uint8_t>& keyIndex = {}) const = 0;
    virtual bool setKey(const std::string& identity,
                        std::vector<uint8_t> key,
                        const std::optional<uint8_t>& keyIndex = {}) = 0;
    virtual std::shared_ptr<E2EKeyHandler> key(const std::string& identity) const = 0;
    virtual std::vector<uint8_t> ratchetKey(const std::string& identity,
                                            const std::optional<uint8_t>& keyIndex = {}) const = 0;
    virtual std::vector<uint8_t> exportKey(const std::string& identity,
                                           const std::optional<uint8_t>& keyIndex = {}) const = 0;
    virtual void setSifTrailer(std::vector<uint8_t> trailer) = 0;
    virtual std::vector<uint8_t> sifTrailer() const = 0;
    virtual const KeyProviderOptions& options() const = 0;
    // string based setters API
    bool setSharedKey(std::string_view key, const std::optional<uint8_t>& keyIndex = {});
    bool setKey(const std::string& identity, std::string_view key,
                const std::optional<uint8_t>& keyIndex = {});
    void setSifTrailer(std::string_view trailer);
};

} // namespace LiveKitCpp
