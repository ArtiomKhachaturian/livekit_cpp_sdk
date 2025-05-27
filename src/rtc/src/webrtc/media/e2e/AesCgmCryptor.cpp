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
#include "AesCgmCryptor.h"
#include "AesCgmCryptorObserver.h"
#include "Utils.h"
#include "livekit/rtc/e2e/KeyProvider.h"
#include "livekit/rtc/e2e/KeyProviderOptions.h"
#include "livekit/rtc/e2e/E2EKeyHandler.h"
#include <common_video/h264/h264_common.h>
#include <common_video/h265/h265_common.h>
#include <openssl/aead.h>
#include <atomic>
#include <map>

namespace
{

using Sinks = std::map<uint32_t, webrtc::scoped_refptr<webrtc::TransformedFrameCallback>>;

const std::string_view g_category("frame_codec");

inline void logInitError(const std::shared_ptr<Bricks::Logger>& logger,
                         std::string_view errorMessage) {
    if (logger && logger->canLogError()) {
        logger->logError(errorMessage, g_category);
    }
}

inline const EVP_AEAD* aesGcmAlgorithmFromKeySize(size_t keySizeBytes)
{
    switch (keySizeBytes) {
        case 16:
            return EVP_aead_aes_128_gcm();
        case 32:
            return EVP_aead_aes_256_gcm();
        default:
            break;
    }
    return nullptr;
}

inline bool needsRbspUnescaping(const uint8_t* frameData, size_t frameSize) {
    for (size_t i = 0; i < frameSize - 3; ++i) {
        if (frameData[i] == 0 &&
            frameData[i + 1] == 0 &&
            frameData[i + 2] == 3) {
            return true;
        }
    }
    return false;
}

uint8_t unencryptedBytes(const webrtc::TransformableFrameInterface* frame, webrtc::MediaType type);
bool frameIsH264(const webrtc::TransformableFrameInterface* frame, webrtc::MediaType type);
bool frameIsH265(const webrtc::TransformableFrameInterface* frame, webrtc::MediaType type);

}

namespace LiveKitCpp
{

AesCgmCryptor::AesCgmCryptor(webrtc::MediaType mediaType,
                             std::string identity,
                             std::string trackId,
                             std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                             const std::shared_ptr<KeyProvider>& keyProvider,
                             const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<webrtc::FrameTransformerInterface>(logger)
    , _mediaType(mediaType)
    , _identity(std::move(identity))
    , _trackId(std::move(trackId))
    , _keyProvider(keyProvider)
    , _logCategory(std::string(g_category) + "_" + _trackId)
    , _observer(std::move(signalingQueue))
{
}

AesCgmCryptor::~AesCgmCryptor()
{
}

webrtc::scoped_refptr<AesCgmCryptor> AesCgmCryptor::
    create(webrtc::MediaType mediaType,
           std::string identity,
           std::string trackId,
           std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
           const std::shared_ptr<KeyProvider>& keyProvider,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!keyProvider) {
        logInitError(logger, "no key provider");
        return {};
    }
    switch (mediaType) {
        case webrtc::MediaType::VIDEO:
        case webrtc::MediaType::AUDIO:
            break;
        default:
            logInitError(logger, "unsupported media type: " + webrtc::MediaTypeToString(mediaType));
            return {};
    }
    if (identity.empty()) {
        logInitError(logger, "unknown identity");
        return {};
    }
    if (trackId.empty()) {
        logInitError(logger, "unknown track ID");
        return {};
    }
    return webrtc::make_ref_counted<AesCgmCryptor>(mediaType, std::move(identity), std::move(trackId),
                                                   std::move(signalingQueue), keyProvider, logger);
}

bool AesCgmCryptor::hasSink() const
{
    LOCK_READ_SAFE_OBJ(_sink);
    return nullptr != _sink->get();
}

bool AesCgmCryptor::hasSinks() const
{
    LOCK_READ_SAFE_OBJ(_sinks);
    return !_sinks->empty();
}

void AesCgmCryptor::Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame) {
        if (!hasSink() && !hasSinks()) {
            if (canLogWarning()) {
                logWarning("no transformation callbacks");
            }
            return;
        }
        switch (frame->GetDirection()) {
            case webrtc::TransformableFrameInterface::Direction::kSender:
                encryptFrame(std::move(frame));
                break;
            case webrtc::TransformableFrameInterface::Direction::kReceiver:
                decryptFrame(std::move(frame));
                break;
            default:
                // do nothing
                break;
        }
    }
}

void AesCgmCryptor::RegisterTransformedFrameCallback(webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback)
{
    if (callback) {
        _sink(std::move(callback));
    }
}

void AesCgmCryptor::RegisterTransformedFrameSinkCallback(webrtc::scoped_refptr<webrtc::TransformedFrameCallback> callback,
                                                         uint32_t ssrc)
{
    if (callback) {
        LOCK_WRITE_SAFE_OBJ(_sinks);
        _sinks->insert(std::make_pair(ssrc, std::move(callback)));
    }
}

void AesCgmCryptor::UnregisterTransformedFrameCallback()
{
    _sink({});
}

void AesCgmCryptor::UnregisterTransformedFrameSinkCallback(uint32_t ssrc)
{
    LOCK_WRITE_SAFE_OBJ(_sinks);
    _sinks->erase(ssrc);
}

void AesCgmCryptor::encryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame) {
        webrtc::scoped_refptr<webrtc::TransformedFrameCallback> sink;
        if (webrtc::MediaType::AUDIO == _mediaType) {
            sink = _sink();
        }
        else {
            LOCK_READ_SAFE_OBJ(_sinks);
            const auto it = _sinks->find(frame->GetSsrc());
            if (it != _sinks->end()) {
                sink = it->second;
            }
        }
        if (!sink) {
            setLastEncryptState(AesCgmCryptorState::InternalError,
                                "unable to encrypt frame, sink is NULL",
                                Bricks::LoggingSeverity::Warning);
            return;
        }
        
        const auto dataIn = frame->GetData();
        if (dataIn.empty()) {
            return;
        }
        if (!_enabledCryption) {
            if (canLogWarning()) {
                logWarning("encryption disabled");
            }
            if (_keyProvider->options()._discardFrameWhenCryptorNotReady) {
                return;
            }
            sink->OnTransformedFrame(std::move(frame));
            return;
        }
        
        const auto keyHandler = this->keyHandler();
        if (!keyHandler) {
            setLastEncryptState(AesCgmCryptorState::MissingKey,
                                "no keys for encrypt",
                                Bricks::LoggingSeverity::Error);
            return;
        }
        
        const auto keyIndex = _keyIndex.load();
        const auto keySet = keyHandler->keySet(keyIndex);
        if (!keySet) {
            setLastEncryptState(AesCgmCryptorState::MissingKey,
                                "no key set for key index " + std::to_string(keyIndex),
                                Bricks::LoggingSeverity::Error);
            return;
        }
        
        const auto frameHeaderSize = unencryptedBytes(frame.get(), _mediaType);
        webrtc::Buffer frameHeader(frameHeaderSize);
        for (size_t i = 0U; i < frameHeaderSize; i++) {
            frameHeader[i] = dataIn[i];
        }
        
        webrtc::Buffer iv = makeIv(frame->GetSsrc(), frame->GetTimestamp());
        
        webrtc::Buffer payload(dataIn.size() - frameHeaderSize);
        for (size_t i = frameHeaderSize; i < dataIn.size(); i++) {
            payload[i - frameHeaderSize] = dataIn[i];
        }
        
        std::vector<uint8_t> buffer;
        if (encryptOrDecrypt(true, keySet->_encryptionKey, iv, frameHeader,
                             payload, buffer)) {
            webrtc::Buffer frameTrailer(2U);
            frameTrailer[0] = ivSize();
            frameTrailer[1] = _keyIndex;
            
            webrtc::Buffer encryptedPayload(buffer.data(), buffer.size());
            webrtc::Buffer tag(encryptedPayload.data() + encryptedPayload.size() - 16, 16);
            
            webrtc::Buffer dataWithoutHeader;
            dataWithoutHeader.AppendData(encryptedPayload);
            dataWithoutHeader.AppendData(iv);
            dataWithoutHeader.AppendData(frameTrailer);

            webrtc::Buffer dataOut;
            dataOut.AppendData(frameHeader);
            if (frameIsH264(frame.get(), _mediaType)) {
                webrtc::H264::WriteRbsp(dataWithoutHeader, &dataOut);
            }
            else if (frameIsH265(frame.get(), _mediaType)) {
                webrtc::H265::WriteRbsp(dataWithoutHeader, &dataOut);
            }
            else {
                dataOut.AppendData(dataWithoutHeader);
                RTC_CHECK_EQ(dataOut.size(), frameHeader.size() +
                             encryptedPayload.size() + iv.size() +
                             frameTrailer.size());
            }
            frame->SetData(dataOut);
            setLastEncryptState(AesCgmCryptorState::Ok);
            sink->OnTransformedFrame(std::move(frame));
        }
        else {
            setLastEncryptState(AesCgmCryptorState::EncryptionFailed,
                                "encrypt frame failed",
                                Bricks::LoggingSeverity::Error);
        }
    }
}

void AesCgmCryptor::decryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame) {
        webrtc::scoped_refptr<webrtc::TransformedFrameCallback> sink;
        if (webrtc::MediaType::AUDIO == _mediaType) {
            sink = _sink();
        }
        else {
            LOCK_READ_SAFE_OBJ(_sinks);
            const auto it = _sinks->find(frame->GetSsrc());
            if (it != _sinks->end()) {
                sink = it->second;
            }
        }
        if (!sink) {
            setLastDecryptState(AesCgmCryptorState::InternalError,
                                "unable to decrypt frame, sink is NULL",
                                Bricks::LoggingSeverity::Warning);
            return;
        }
        
        const auto dataIn = frame->GetData();
        if (dataIn.empty() || !_enabledCryption) {
            if (canLogWarning()) {
                logWarning("no input data for decrypt or decryption disabled");
            }
            if (_keyProvider->options()._discardFrameWhenCryptorNotReady) {
                return;
            }
            sink->OnTransformedFrame(std::move(frame));
            return;
        }
        
        const auto sif = _keyProvider->sifTrailer();
        if (!sif.empty() && dataIn.size() >= sif.size()) {
            auto tmp = dataIn.subview(dataIn.size() - (sif.size()), sif.size());
            std::vector<uint8_t> data(tmp.begin(), tmp.end());
            if (data == sif) {
                // magic bytes detected, this is a non-encrypted frame,
                // skip frame decryption
                webrtc::Buffer dataOut;
                dataOut.AppendData(dataIn.subview(0, dataIn.size() - sif.size()));
                frame->SetData(dataOut);
                sink->OnTransformedFrame(std::move(frame));
                return;
            }
        }
        
        const auto frameHeaderSize = unencryptedBytes(frame.get(), _mediaType);
        webrtc::Buffer frameHeader(frameHeaderSize);
        for (size_t i = 0U; i < frameHeaderSize; i++) {
            frameHeader[i] = dataIn[i];
        }
        
        webrtc::Buffer frameTrailer(2);
        frameTrailer[0] = dataIn[dataIn.size() - 2];
        frameTrailer[1] = dataIn[dataIn.size() - 1];
        uint8_t ivLength = frameTrailer[0];
        uint8_t keyIndex = frameTrailer[1];
        if (ivLength != ivSize()) {
            setLastDecryptState(AesCgmCryptorState::DecryptionFailed,
                                "incorrect IV size for decryption",
                                Bricks::LoggingSeverity::Warning);
            return;
        }
        
        const auto keyHandler = this->keyHandler();
        if (!keyHandler) {
            setLastEncryptState(AesCgmCryptorState::MissingKey,
                                "no keys for decrypt",
                                Bricks::LoggingSeverity::Error);
            return;
        }
        
        if (keyIndex >= _keyProvider->options()._keyRingSize) {
            setLastDecryptState(AesCgmCryptorState::MissingKey,
                                "decryption key index [" +
                                std::to_string(keyIndex) +
                                "] is out of range ",
                                Bricks::LoggingSeverity::Error);
            return;
        }

        const auto keySet = keyHandler->keySet(keyIndex);
        if (!keySet) {
            setLastDecryptState(AesCgmCryptorState::MissingKey,
                                "no key set for decryption key index [" +
                                std::to_string(keyIndex) + "]",
                                Bricks::LoggingSeverity::Error);
            return;
        }
        
        if (AesCgmCryptorState::DecryptionFailed == _lastDecState && !keyHandler->hasValidKey()) {
            // if decryption failed and we have an invalid key,
            // please try to decrypt with the next new key
            return;
        }
        
        webrtc::Buffer iv(ivLength);
        for (size_t i = 0U; i < ivLength; i++) {
            iv[i] = dataIn[dataIn.size() - 2 - ivLength + i];
        }
        
        webrtc::Buffer encryptedBuffer(dataIn.size() - frameHeaderSize);
        for (size_t i = frameHeaderSize; i < dataIn.size(); i++) {
            encryptedBuffer[i - frameHeaderSize] = dataIn[i];
        }
        
        if (frameIsH264(frame.get(), _mediaType)) {
            if (needsRbspUnescaping(encryptedBuffer.data(), encryptedBuffer.size())) {
                webrtc::H264::ParseRbsp(encryptedBuffer.data(), encryptedBuffer.size());
            }
        }
        else if (frameIsH265(frame.get(), _mediaType)) {
            if (needsRbspUnescaping(encryptedBuffer.data(), encryptedBuffer.size())) {
                webrtc::H265::ParseRbsp(encryptedBuffer.data(), encryptedBuffer.size());
            }
        }
        
        webrtc::Buffer encryptedPayload(encryptedBuffer.size() - ivLength - 2);
        for (size_t i = 0U; i < encryptedPayload.size(); i++) {
            encryptedPayload[i] = encryptedBuffer[i];
        }

        webrtc::Buffer tag(encryptedPayload.data() + encryptedPayload.size() - 16, 16);
        std::vector<uint8_t> buffer;
        auto initialKeyMaterial = keySet->_material;
        bool decryptionSuccess = encryptOrDecrypt(false, keySet->_encryptionKey,
                                                  iv, frameHeader,
                                                  encryptedPayload, buffer);
        if (!decryptionSuccess) {
            if (canLogWarning()) {
                logWarning("decrypt frame failed");
            }
            std::shared_ptr<KeySet> ratchetedKeySet;
            size_t ratchetCount = 0U;
            auto currentKeyMaterial = keySet->_material;
            if (_keyProvider->options()._ratchetWindowSize > 0U) {
                while (ratchetCount < _keyProvider->options()._ratchetWindowSize) {
                    ratchetCount++;
                    if (canLogVerbose()) {
                        logVerbose("ratcheting key attempt " +
                                   std::to_string(ratchetCount) + " of " +
                                   std::to_string(_keyProvider->options()._ratchetWindowSize));
                    }
                    auto newMaterial = keyHandler->ratchetKeyMaterial(currentKeyMaterial);
                    ratchetedKeySet = keyHandler->deriveKeys(newMaterial,
                                                             _keyProvider->options()._ratchetSalt,
                                                             128);
                    decryptionSuccess = encryptOrDecrypt(false,
                                                         ratchetedKeySet->_encryptionKey,
                                                         iv, frameHeader,
                                                         encryptedPayload, buffer);
                    if (decryptionSuccess) {
                        // success, so we set the new key
                        keyHandler->setKeyFromMaterial(newMaterial, keyIndex);
                        keyHandler->setHasValidKey();
                        setLastDecryptState(AesCgmCryptorState::KeyRatcheted);
                        break;
                    }
                    // for the next ratchet attempt
                    currentKeyMaterial = newMaterial;
                } // while (ratchetCount < _keyProvider->options()._ratchetWindowSize)
                /* Since the key it is first send and only afterwards actually used for
                   encrypting, there were situations when the decrypting failed due to the
                   fact that the received frame was not encrypted yet and ratcheting, of
                   course, did not solve the problem. So if we fail RATCHET_WINDOW_SIZE
                   times, we come back to the initial key.
                 */
                if (!decryptionSuccess && ratchetCount >= _keyProvider->options()._ratchetWindowSize) {
                    keyHandler->setKeyFromMaterial(std::move(initialKeyMaterial), keyIndex);
                }
            } //  if (_keyProvider->options()._ratchetWindowSize > 0)
        }
        
        if (!decryptionSuccess) {
            if (keyHandler->decryptionFailure()) {
                setLastDecryptState(AesCgmCryptorState::DecryptionFailed);
            }
            return;
        }
        
        webrtc::Buffer payload(buffer.data(), buffer.size());
        webrtc::Buffer dataOut;
        dataOut.AppendData(frameHeader);
        dataOut.AppendData(payload);
        frame->SetData(dataOut);
        
        setLastDecryptState(AesCgmCryptorState::Ok);
        sink->OnTransformedFrame(std::move(frame));
    }
}

void AesCgmCryptor::setLastEncryptState(AesCgmCryptorState state,
                                        const std::string& comment,
                                        Bricks::LoggingSeverity severity)
{
    if (exchangeVal(state, _lastEncState)) {
        if (!comment.empty() && canLog(severity)) {
            log(severity, comment);
        }
        _observer.invoke(&AesCgmCryptorObserver::onEncryptionStateChanged,
                         _mediaType, _identity, _trackId, state);
    }
}

void AesCgmCryptor::setLastDecryptState(AesCgmCryptorState state,
                                        const std::string& comment,
                                        Bricks::LoggingSeverity severity)
{
    if (exchangeVal(state, _lastDecState)) {
        if (!comment.empty() && canLog(severity)) {
            log(severity, comment);
        }
        _observer.invoke(&AesCgmCryptorObserver::onDecryptionStateChanged,
                         _mediaType, _identity, _trackId, state);
    }
}

std::shared_ptr<E2EKeyHandler> AesCgmCryptor::keyHandler() const
{
    if (_keyProvider->options()._sharedKey) {
        return _keyProvider->sharedKey(_identity);
    }
    return _keyProvider->key(_identity);
}

bool AesCgmCryptor::encryptOrDecrypt(bool encrypt,
                                     const std::vector<uint8_t>& rawKey,
                                     const webrtc::ArrayView<uint8_t>& iv,
                                     const webrtc::ArrayView<uint8_t>& additionalData,
                                     const webrtc::ArrayView<uint8_t>& data,
                                     std::vector<uint8_t>& buffer) const
{
    const EVP_AEAD* aeadAlg = aesGcmAlgorithmFromKeySize(rawKey.size());
    if (!aeadAlg) {
        if (canLogError()) {
            logError("invalid AES-GCM key size");
        }
        return false;
    }
    static constexpr unsigned tagLengthBytes = 128U / 8U;
    
    bssl::ScopedEVP_AEAD_CTX ctx;
    if (!EVP_AEAD_CTX_init(ctx.get(), aeadAlg,
                           rawKey.data(), rawKey.size(),
                           tagLengthBytes, nullptr)) {
        if (canLogError()) {
            logError("failed to initialize AES-GCM context");
        }
        return false;
    }
    size_t len = {};
    int ok = {};
    if (encrypt) {
        buffer.resize(data.size() + EVP_AEAD_max_overhead(aeadAlg));
        ok = EVP_AEAD_CTX_seal(ctx.get(), buffer.data(), &len, buffer.size(),
                               iv.data(), iv.size(), data.data(), data.size(),
                               additionalData.data(), additionalData.size());
    }
    else {
        if (data.size() < tagLengthBytes) {
            if (canLogError()) {
                logError("data too small for AES-GCM tag");
            }
            return false;
        }
        buffer.resize(data.size() - tagLengthBytes);
        ok = EVP_AEAD_CTX_open(ctx.get(), buffer.data(), &len, buffer.size(),
                               iv.data(), iv.size(), data.data(), data.size(),
                               additionalData.data(), additionalData.size());
    }
    if (!ok) {
        if (canLogWarning()) {
            logWarning("failed to perform AES-GCM operation");
        }
        return false;
    }
    buffer.resize(len);
    return true;
}

webrtc::Buffer AesCgmCryptor::makeIv(uint32_t ssrc, uint32_t timestamp)
{
    uint32_t sendCount = 0U;
    const auto it = _sendCounts.find(ssrc);
    if (it == _sendCounts.end()) {
        srand((unsigned)time(nullptr));
        _sendCounts[ssrc] = floor(rand() * 0xFFFF);
    }
    else {
        sendCount = it->second;
    }
    webrtc::ByteBufferWriter buf;
    buf.WriteUInt32(ssrc);
    buf.WriteUInt32(timestamp);
    buf.WriteUInt32(timestamp - (sendCount % 0xFFFF));
    _sendCounts[ssrc] = sendCount + 1;
    RTC_CHECK_EQ(buf.Length(), ivSize());
    return webrtc::Buffer(buf.Data(), buf.Length());
}

std::optional<E2ECryptoError> toCryptoError(AesCgmCryptorState state)
{
    switch (state) {
        case AesCgmCryptorState::EncryptionFailed:
            return E2ECryptoError::EncryptionFailed;
        case AesCgmCryptorState::DecryptionFailed:
            return E2ECryptoError::DecryptionFailed;
        case AesCgmCryptorState::MissingKey:
            return E2ECryptoError::MissingKey;
        case AesCgmCryptorState::InternalError:
            return E2ECryptoError::InternalError;
        default:
            break;
    }
    return std::nullopt;
}

} // namespace LiveKitCpp

namespace
{

uint8_t unencryptedH264Bytes(const webrtc::ArrayView<const uint8_t>& data)
{
    const auto indices = webrtc::H264::FindNaluIndices(data);
    for (const auto& index : indices) {
        const auto slice = data.data() + index.payload_start_offset;
        switch (webrtc::H264::ParseNaluType(slice[0])) {
            case webrtc::H264::NaluType::kIdr:
            case webrtc::H264::NaluType::kSlice:
                return index.payload_start_offset + 2;
            default:
                break;
        }
    }
    return 12; // ?
}

uint8_t unencryptedH265Bytes(const webrtc::ArrayView<const uint8_t>& data)
{
    const auto indices = webrtc::H265::FindNaluIndices(data);
    for (const auto& index : indices) {
        const auto slice = data.data() + index.payload_start_offset;
        switch (webrtc::H265::ParseNaluType(slice[0])) {
            case webrtc::H265::NaluType::kIdrWRadl:
            case webrtc::H265::NaluType::kIdrNLp:
            case webrtc::H265::NaluType::kCra:
                return index.payload_start_offset + 3;
            default:
                break;
        }
    }
    return 12; // ?
}

uint8_t unencryptedBytes(const webrtc::TransformableFrameInterface* frame, webrtc::MediaType type)
{
    if (frame) {
        if (webrtc::MediaType::AUDIO == type) {
            return 1; // opus
        }
        if (webrtc::MediaType::VIDEO == type) {
            const auto videoFrame = static_cast<const webrtc::TransformableVideoFrameInterface*>(frame);
            const auto metaData = videoFrame->Metadata();
            switch (metaData.GetCodec()) {
                case webrtc::kVideoCodecVP8:
                    // a 3-octet version for interframes and a 10-octet version for key frames
                    // https://www.rfc-editor.org/rfc/rfc7741 (VP8 Payload Header)
                    if (webrtc::VideoFrameType::kVideoFrameKey == metaData.GetFrameType()) {
                        return 10;
                    }
                    return 3;
                case webrtc::kVideoCodecH264:
                    return unencryptedH264Bytes(frame->GetData());
                case webrtc::kVideoCodecH265:
                    return unencryptedH265Bytes(frame->GetData());
                default:
                    break;
            }
        }
    }
    return 0;
}

inline bool checkVideoCodecType(const webrtc::TransformableFrameInterface* frame,
                                webrtc::MediaType mediaType,
                                webrtc::VideoCodecType codecType)
{
    if (frame && webrtc::MediaType::VIDEO == mediaType) {
        const auto videoFrame = static_cast<const webrtc::TransformableVideoFrameInterface*>(frame);
        return codecType == videoFrame->Metadata().GetCodec();
    }
    return false;
}

bool frameIsH264(const webrtc::TransformableFrameInterface* frame, webrtc::MediaType type)
{
    return checkVideoCodecType(frame, type, webrtc::kVideoCodecH264);
}

bool frameIsH265(const webrtc::TransformableFrameInterface* frame, webrtc::MediaType type)
{
    return checkVideoCodecType(frame, type, webrtc::kVideoCodecH265);
}

}
