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
#include "e2e/ParticipantKeyHandler.h"
#include "Loggable.h"
#include "SafeObj.h"
#include <unordered_map>

namespace  {

const std::string g_shared("shared");

}

namespace LiveKitCpp
{

using ParticipantKeyHandlers = std::unordered_map<std::string, std::shared_ptr<ParticipantKeyHandler>>;

struct DefaultKeyProvider::Impl : public Bricks::LoggableS<>
{
    const KeyProviderOptions _options;
    Bricks::SafeObj<ParticipantKeyHandlers> _keys;
    Bricks::SafeObj<std::vector<uint8_t>> _uncryptedMagicBytes; // sif
    Impl(KeyProviderOptions options, const std::shared_ptr<Bricks::Logger>& logger);
    std::shared_ptr<ParticipantKeyHandler> newKeyHandler() const;
    std::shared_ptr<ParticipantKeyHandler> emplaceHandler(const std::string& id);
};

DefaultKeyProvider::DefaultKeyProvider(KeyProviderOptions options,
                                       const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(std::make_unique<Impl>(std::move(options), logger))
{
}

DefaultKeyProvider::~DefaultKeyProvider()
{
}

bool DefaultKeyProvider::setSharedKey(std::vector<uint8_t> key,
                                      const std::optional<uint8_t>& keyIndex)
{
    if (_impl->_options._sharedKey) {
        std::shared_ptr<ParticipantKeyHandler> keyHandler;
        {
            LOCK_WRITE_SAFE_OBJ(_impl->_keys);
            keyHandler = _impl->emplaceHandler(g_shared);
            if (keyHandler) {
                for (auto it = _impl->_keys->begin(); it != _impl->_keys->end(); ++it) {
                    if (it->first != g_shared) {
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

std::shared_ptr<ParticipantKeyHandler> DefaultKeyProvider::
    sharedKey(const std::string& participantSid)
{
    if (_impl->_options._sharedKey && !participantSid.empty()) {
        LOCK_WRITE_SAFE_OBJ(_impl->_keys);
        auto it = _impl->_keys->find(participantSid);
        if (it != _impl->_keys->end()) {
            return it->second;
        }
        it = _impl->_keys->find(g_shared);
        if (it != _impl->_keys->end()) {
            const auto keyHandler = it->second->clone();
            _impl->_keys->insert(std::make_pair(participantSid, keyHandler));
            return keyHandler;
        }
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::ratchetSharedKey(const std::optional<uint8_t>& keyIndex)
{
    LOCK_READ_SAFE_OBJ(_impl->_keys);
    auto it = _impl->_keys->find(g_shared);
    if (it != _impl->_keys->end()) {
        auto newKey = it->second->ratchetKey(keyIndex);
        if (_impl->_options._sharedKey) {
            for (it = _impl->_keys->begin(); it != _impl->_keys->end(); ++it) {
                if (it->first != g_shared) {
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
    LOCK_READ_SAFE_OBJ(_impl->_keys);
    auto it = _impl->_keys->find(g_shared);
    if (it != _impl->_keys->end()) {
        if (const auto keySet = it->second->keySet(keyIndex)) {
            return keySet->_material;
        }
    }
    return {};
}

bool DefaultKeyProvider::setKey(const std::string& participantSid,
                                std::vector<uint8_t> key,
                                const std::optional<uint8_t>& keyIndex)
{
    if (!participantSid.empty()) {
        std::shared_ptr<ParticipantKeyHandler> keyHandler;
        {
            LOCK_WRITE_SAFE_OBJ(_impl->_keys);
            keyHandler = _impl->emplaceHandler(participantSid);
        }
        if (keyHandler) {
            keyHandler->setKey(std::move(key), keyIndex);
            return true;
        }
    }
    return false;
}

std::shared_ptr<ParticipantKeyHandler> DefaultKeyProvider::key(const std::string& participantSid) const
{
    if (!participantSid.empty()) {
        LOCK_READ_SAFE_OBJ(_impl->_keys);
        const auto it = _impl->_keys->find(participantSid);
        if (it != _impl->_keys->end()) {
            return it->second;
        }
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::ratchetKey(const std::string& participantSid,
                                                    const std::optional<uint8_t>& keyIndex) const
{
    if (const auto keyHandler = key(participantSid)) {
        return keyHandler->ratchetKey(keyIndex);
    }
    return {};
}

std::vector<uint8_t> DefaultKeyProvider::exportKey(const std::string& participantSid,
                                                   const std::optional<uint8_t>& keyIndex) const
{
    if (const auto keyHandler = key(participantSid)) {
        if (const auto keySet = keyHandler->keySet(keyIndex)) {
            return keySet->_material;
        }
    }
    return {};
}

void DefaultKeyProvider::setSifTrailer(std::vector<uint8_t> trailer)
{
    _impl->_uncryptedMagicBytes(std::move(trailer));
}

std::vector<uint8_t> DefaultKeyProvider::sifTrailer() const
{
    return _impl->_uncryptedMagicBytes();
}

const KeyProviderOptions& DefaultKeyProvider::options() const
{
    return _impl->_options;
}

DefaultKeyProvider::Impl::Impl(KeyProviderOptions options,
                               const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _options(std::move(options))
{
}

std::shared_ptr<ParticipantKeyHandler> DefaultKeyProvider::Impl::newKeyHandler() const
{
    return std::make_shared<ParticipantKeyHandler>(_options, logger());
}

std::shared_ptr<ParticipantKeyHandler> DefaultKeyProvider::Impl::emplaceHandler(const std::string& id)
{
    std::shared_ptr<ParticipantKeyHandler> keyHandler;
    if (!id.empty()) {
        auto it = _keys->find(id);
        if (it == _keys->end()) {
            keyHandler = newKeyHandler();
            _keys->insert(std::make_pair(id, keyHandler));
        }
        else {
            keyHandler = it->second;
        }
    }
    return keyHandler;
}

} // namespace LiveKitCpp
