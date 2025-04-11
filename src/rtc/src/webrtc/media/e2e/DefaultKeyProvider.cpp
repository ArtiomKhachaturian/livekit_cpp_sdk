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
#include "e2e/DefaultKeyProvider.h"
#include "e2e/E2EKeyHandler.h"

namespace LiveKitCpp
{

DefaultKeyProvider::DefaultKeyProvider(KeyProviderOptions options,
                                       const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<KeyProvider>(logger)
    , _options(std::move(options))
{
}

DefaultKeyProvider::~DefaultKeyProvider()
{
}

bool DefaultKeyProvider::setSharedKey(std::vector<uint8_t> key,
                                      const std::optional<uint8_t>& keyIndex)
{
    if (_options._sharedKey) {
        std::shared_ptr<E2EKeyHandler> keyHandler;
        {
            LOCK_WRITE_SAFE_OBJ(_keys);
            keyHandler = emplaceHandler(_shared);
            if (keyHandler) {
                for (auto it = _keys->begin(); it != _keys->end(); ++it) {
                    if (it->first != _shared) {
                        it->second->setKey(key, keyIndex);
                    }
                }
            }
        }
        if (keyHandler) {
            keyHandler->setKey(std::move(key), keyIndex);
            return true;
        }
    }
    return false;
}

std::shared_ptr<E2EKeyHandler> DefaultKeyProvider::sharedKey(const std::string& identity)
{
    if (_options._sharedKey && !identity.empty()) {
        LOCK_WRITE_SAFE_OBJ(_keys);
        auto it = _keys->find(identity);
        if (it != _keys->end()) {
            return it->second;
        }
        it = _keys->find(_shared);
        if (it != _keys->end()) {
            const auto keyHandler = it->second->clone();
            _keys->insert(std::make_pair(identity, keyHandler));
            return keyHandler;
        }
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::ratchetSharedKey(const std::optional<uint8_t>& keyIndex)
{
    LOCK_READ_SAFE_OBJ(_keys);
    auto it = _keys->find(_shared);
    if (it != _keys->end()) {
        auto newKey = it->second->ratchetKey(keyIndex);
        if (_options._sharedKey) {
            for (it = _keys->begin(); it != _keys->end(); ++it) {
                if (it->first != _shared) {
                    it->second->setKey(newKey, keyIndex);
                }
            }
        }
        return newKey;
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::exportSharedKey(const std::optional<uint8_t>& keyIndex) const
{
    LOCK_READ_SAFE_OBJ(_keys);
    auto it = _keys->find(_shared);
    if (it != _keys->end()) {
        if (const auto keySet = it->second->keySet(keyIndex)) {
            return keySet->_material;
        }
    }
    return {};
}

bool DefaultKeyProvider::setKey(const std::string& identity,
                                std::vector<uint8_t> key,
                                const std::optional<uint8_t>& keyIndex)
{
    if (!identity.empty()) {
        std::shared_ptr<E2EKeyHandler> keyHandler;
        {
            LOCK_WRITE_SAFE_OBJ(_keys);
            keyHandler = emplaceHandler(identity);
        }
        if (keyHandler) {
            keyHandler->setKey(std::move(key), keyIndex);
            return true;
        }
    }
    return false;
}

std::shared_ptr<E2EKeyHandler> DefaultKeyProvider::key(const std::string& identity) const
{
    if (!identity.empty()) {
        LOCK_READ_SAFE_OBJ(_keys);
        const auto it = _keys->find(identity);
        if (it != _keys->end()) {
            return it->second;
        }
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::ratchetKey(const std::string& identity,
                                                    const std::optional<uint8_t>& keyIndex) const
{
    if (const auto keyHandler = key(identity)) {
        return keyHandler->ratchetKey(keyIndex);
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::exportKey(const std::string& identity,
                                                   const std::optional<uint8_t>& keyIndex) const
{
    if (const auto keyHandler = key(identity)) {
        if (const auto keySet = keyHandler->keySet(keyIndex)) {
            return keySet->_material;
        }
    }
    return {};
}

std::shared_ptr<E2EKeyHandler> DefaultKeyProvider::newKeyHandler() const
{
    return std::make_shared<E2EKeyHandler>(_options, logger());
}

std::shared_ptr<E2EKeyHandler> DefaultKeyProvider::emplaceHandler(const std::string& identity)
{
    std::shared_ptr<E2EKeyHandler> keyHandler;
    if (!identity.empty()) {
        auto it = _keys->find(identity);
        if (it == _keys->end()) {
            keyHandler = newKeyHandler();
            _keys->insert(std::make_pair(identity, keyHandler));
        }
        else {
            keyHandler = it->second;
        }
    }
    return keyHandler;
}

} // namespace LiveKitCpp
