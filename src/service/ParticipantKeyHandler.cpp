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
#include "e2e/ParticipantKeyHandler.h"
#ifdef WEBRTC_AVAILABLE
#include "e2e/KeyProviderOptions.h"
#include "Loggable.h"
#include "SafeObj.h"
#include "Utils.h"
#include <openssl/evp.h>
#include <cassert>
#include <sstream>

namespace {

struct Data
{
    bool _hasValidKey = false;
    uint64_t _decryptionFailureCount = 0ULL;
    size_t _currentKeyIndex = 0U;
    std::vector<std::shared_ptr<LiveKitCpp::KeySet>> _cryptoKeyRing;
    inline size_t boundIndex(size_t index) const {
        return std::min(_cryptoKeyRing.size(), index);
    }
    inline size_t index(const std::optional<size_t>& keyIndex) const {
        return keyIndex ? boundIndex(keyIndex.value()) : _currentKeyIndex;
    }
};

inline std::string toUint8List(const uint8_t* data, size_t len) {
    std::stringstream ss;
    ss << "[";
    if (data && len) {
        for (size_t i = 0U; i < len; i++) {
            ss << static_cast<unsigned>(data[i]) << ",";
        }
    }
    ss << "]";
    return ss.str();
}

inline std::string toUint8List(const std::vector<uint8_t>& data) {
    return toUint8List(data.data(), data.size());
}

}

namespace LiveKitCpp
{

struct ParticipantKeyHandler::Impl : public Bricks::LoggableS<>
{
    const KeyProviderOptions _options;
    Bricks::SafeObj<Data> _data;
    Impl(const KeyProviderOptions& options, const std::shared_ptr<Bricks::Logger>& logger);
    std::unique_ptr<Impl> clone() const;
    bool derivePBKDF2KeyFromRawKey(const std::vector<uint8_t>& rawKey,
                                   const std::vector<uint8_t>& salt,
                                   unsigned int optionalLengthBits,
                                   std::vector<uint8_t>& derivedKey) const;
protected:
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
};

ParticipantKeyHandler::ParticipantKeyHandler(const KeyProviderOptions& options,
                                             const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(std::make_unique<Impl>(options, logger))
{
    auto keyRingSize = options._keyRingSize;
    if (0 == keyRingSize) {
        keyRingSize = KeyProviderOptions::_defaultKeyRingSize;
    }
    else if (keyRingSize > KeyProviderOptions::_maxKeyRingSize) {
        keyRingSize = KeyProviderOptions::_maxKeyRingSize;
    }
    _impl->_data->_cryptoKeyRing.resize(keyRingSize);
}

ParticipantKeyHandler::ParticipantKeyHandler(std::unique_ptr<Impl> impl)
    : _impl(std::move(impl))
{
}

ParticipantKeyHandler::~ParticipantKeyHandler()
{
}

std::shared_ptr<ParticipantKeyHandler> ParticipantKeyHandler::clone() const
{
    return std::shared_ptr<ParticipantKeyHandler>(new ParticipantKeyHandler(_impl->clone()));
}

std::vector<uint8_t> ParticipantKeyHandler::ratchetKey(const std::optional<size_t>& keyIndex)
{
    if (const auto keySet = this->keySet(keyIndex)) {
        std::vector<uint8_t> newMaterial;
        if (_impl->derivePBKDF2KeyFromRawKey(keySet->_material,
                                             _impl->_options._ratchetSalt, 256U,
                                             newMaterial)) {
            setKeyFromMaterial(newMaterial, keyIndex);
            setHasValidKey();
            return newMaterial;
        }
    }
    return {};
}

std::shared_ptr<KeySet> ParticipantKeyHandler::keySet(const std::optional<size_t>& keyIndex) const
{
    LOCK_READ_SAFE_OBJ(_impl->_data);
    return _impl->_data->_cryptoKeyRing.at(_impl->_data->index(keyIndex));
}

void ParticipantKeyHandler::setKey(std::vector<uint8_t> password,
                                   const std::optional<size_t>& keyIndex)
{
    setKeyFromMaterial(std::move(password), keyIndex);
    setHasValidKey();
}

std::vector<uint8_t> ParticipantKeyHandler::
    ratchetKeyMaterial(const std::vector<uint8_t>& currentMaterial) const
{
    std::vector<uint8_t> newMaterial;
    if (_impl->derivePBKDF2KeyFromRawKey(currentMaterial, _impl->_options._ratchetSalt,
                                         256U, newMaterial)) {
        return newMaterial;
    }
    return {};
}

std::shared_ptr<KeySet> ParticipantKeyHandler::
    deriveKeys(const std::vector<uint8_t>& ratchetSalt, std::vector<uint8_t> password,
               unsigned int optionalLengthBits) const
{
    std::vector<uint8_t> derivedKey;
    if (_impl->derivePBKDF2KeyFromRawKey(password, ratchetSalt,
                                         optionalLengthBits, derivedKey)) {
        return std::make_shared<KeySet>(std::move(password), std::move(derivedKey));
    }
    return {};
}

bool ParticipantKeyHandler::hasValidKey() const
{
    LOCK_READ_SAFE_OBJ(_impl->_data);
    return _impl->_data->_hasValidKey;
}

void ParticipantKeyHandler::setHasValidKey()
{
    LOCK_WRITE_SAFE_OBJ(_impl->_data);
    _impl->_data->_decryptionFailureCount = 0U;
    _impl->_data->_hasValidKey = true;
}

void ParticipantKeyHandler::setKeyFromMaterial(std::vector<uint8_t> password,
                                               const std::optional<size_t>& keyIndex)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_data);
    auto& data = _impl->_data;
    if (keyIndex.has_value()) {
        const auto index = data->boundIndex(keyIndex.value());
        data->_currentKeyIndex = index % data->_cryptoKeyRing.size();
    }
    data->_cryptoKeyRing[data->_currentKeyIndex] = deriveKeys(_impl->_options._ratchetSalt,
                                                              std::move(password),
                                                              128U);
}

bool ParticipantKeyHandler::decryptionFailure()
{
    const auto& options = _impl->_options;
    if (!options._failureTolerance.has_value()) {
        return false;
    }
    LOCK_WRITE_SAFE_OBJ(_impl->_data);
    auto& decryptionFailureCount = _impl->_data->_decryptionFailureCount;
    auto& hasValidKey = _impl->_data->_hasValidKey;
    decryptionFailureCount++;
    hasValidKey = decryptionFailureCount < options._failureTolerance.value();
    return !hasValidKey;
    return true;
}

ParticipantKeyHandler::Impl::Impl(const KeyProviderOptions& options,
                                  const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _options(options)
{
}

std::unique_ptr<ParticipantKeyHandler::Impl> ParticipantKeyHandler::Impl::clone() const
{
    auto impl = std::make_unique<Impl>(_options, logger());
    impl->_data = _data();
    return impl;
}

bool ParticipantKeyHandler::Impl::derivePBKDF2KeyFromRawKey(const std::vector<uint8_t>& rawKey,
                                                            const std::vector<uint8_t>& salt,
                                                            unsigned int optionalLengthBits,
                                                            std::vector<uint8_t>& derivedKey) const
{
    const size_t keySizeBytes = optionalLengthBits / 8;
    derivedKey.resize(keySizeBytes);
    const auto res = PKCS5_PBKDF2_HMAC((const char*)rawKey.data(), int(rawKey.size()),
                                       salt.data(), int(salt.size()),
                                       100000, EVP_sha256(),
                                       int(keySizeBytes), derivedKey.data());
    if (1 != res) {
        if (canLogError()) {
            logError("failed to derive AES key from password, error code: " + std::to_string(res));
        }
        return false;
    }
    if (canLogVerbose()) {
        logVerbose("raw_key " + toUint8List(rawKey) +
                   " len " + std::to_string(rawKey.size()) +
                   "salt " + toUint8List(salt) +
                   " len " + std::to_string(salt.size()));
        logVerbose("derived_key " + toUint8List(derivedKey) +
                   " len " + std::to_string(derivedKey.size()));
    }
    return true;
}


std::string_view ParticipantKeyHandler::Impl::logCategory() const
{
    static const std::string_view category("ParticipantKeyHandler");
    return category;
}

} // namespace LiveKitCpp
#endif
