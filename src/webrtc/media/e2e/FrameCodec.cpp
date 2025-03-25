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
#include "FrameCodec.h"
#include "FrameCodecState.h"
#include "FrameCodecObserver.h"
#include "AsyncListener.h"
#include "Listener.h"
#include "Loggable.h"
#include "SafeScopedRefPtr.h"
#include "Utils.h"
#include "e2e/KeyProvider.h"
#include "e2e/KeyProviderOptions.h"
#include "e2e/ParticipantKeyHandler.h"
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
    if (logger) {
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

uint8_t unencryptedBytes(const webrtc::TransformableFrameInterface* frame, cricket::MediaType type);
bool frameIsH264(const webrtc::TransformableFrameInterface* frame, cricket::MediaType type);
bool frameIsH265(const webrtc::TransformableFrameInterface* frame, cricket::MediaType type);

}

namespace LiveKitCpp
{

struct FrameCodec::Impl : public Bricks::LoggableS<>
{
    // data
    const cricket::MediaType _mediaType;
    const std::string _trackId;
    const std::shared_ptr<KeyProvider> _keyProvider;
    const std::string _logCategory;
    std::atomic<uint8_t> _keyIndex = 0;
    std::atomic_bool _enabledCryption = true;
    AsyncListener<FrameCodecObserver*, true> _observer;
    SafeScopedRefPtr<webrtc::TransformedFrameCallback> _sink;
    Bricks::SafeObj<Sinks> _sinks;
    // methods
    Impl(cricket::MediaType mediaType,
         std::string trackId,
         const std::weak_ptr<rtc::Thread>& signalingThread,
         const std::shared_ptr<KeyProvider>& keyProvider,
         const std::shared_ptr<Bricks::Logger>& logger);
    bool hasSink() const;
    bool hasSinks() const;
    void transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame);
protected:
    // override of Bricks::LoggableS<>
    std::string_view logCategory() const final { return _logCategory; }
private:
    void encryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame);
    void decryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame);
    void setLastEncryptState(FrameCodecState state);
    void setLastDecryptState(FrameCodecState state);
    std::shared_ptr<ParticipantKeyHandler> keyHandler() const;
    bool aesGcmEncryptDecrypt(bool encrypt,
                              const std::vector<uint8_t>& rawKey,
                              const rtc::ArrayView<uint8_t>& data,
                              const rtc::ArrayView<uint8_t>& iv,
                              const rtc::ArrayView<uint8_t>& additionalData,
                              std::vector<uint8_t>& buffer) const;
    static constexpr uint8_t ivSize() noexcept { return 12; }
    rtc::Buffer makeIv(uint32_t ssrc, uint32_t timestamp);
private:
    std::atomic<FrameCodecState> _lastEncState = FrameCodecState::New;
    std::atomic<FrameCodecState> _lastDecState = FrameCodecState::New;
    static thread_local inline std::map<uint32_t, uint32_t> _sendCounts;
};

FrameCodec::FrameCodec(cricket::MediaType mediaType,
                       std::string trackId,
                       const std::weak_ptr<rtc::Thread>& signalingThread,
                       const std::shared_ptr<KeyProvider>& keyProvider,
                       const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(std::make_shared<Impl>(mediaType, std::move(trackId),
                                   signalingThread, keyProvider, logger))
    , _queue(createTaskQueueU(_impl->_logCategory, webrtc::TaskQueueFactory::Priority::HIGH))
{
}

FrameCodec::~FrameCodec()
{
}

void FrameCodec::setKeyIndex(uint8_t keyIndex)
{
    _impl->_keyIndex = keyIndex;
}

uint8_t FrameCodec::keyIndex() const
{
    return _impl->_keyIndex;
}

void FrameCodec::setEnabled(bool enabled)
{
    _impl->_enabledCryption = enabled;
}

bool FrameCodec::enabled() const
{
    return _impl->_enabledCryption;
}

void FrameCodec::setObserver(FrameCodecObserver* observer)
{
    _impl->_observer.set(observer);
}

void FrameCodec::Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame && _queue) {
        if (!_impl->hasSink() && !_impl->hasSinks()) {
            _impl->logWarning("no transformation callbacks");
            return;
        }
        // do encrypt or decrypt here...
        _queue->PostTask([frame = std::move(frame),
                          weakRef = std::weak_ptr<Impl>(_impl)]() mutable {
            if (const auto self = weakRef.lock()) {
                self->transform(std::move(frame));
            }
        });
    }
}

void FrameCodec::RegisterTransformedFrameCallback(rtc::scoped_refptr<webrtc::TransformedFrameCallback> callback)
{
    if (callback) {
        _impl->_sink(std::move(callback));
    }
}

void FrameCodec::RegisterTransformedFrameSinkCallback(rtc::scoped_refptr<webrtc::TransformedFrameCallback> callback,
                                                      uint32_t ssrc)
{
    if (callback) {
        LOCK_WRITE_SAFE_OBJ(_impl->_sinks);
        _impl->_sinks->insert(std::make_pair(ssrc, std::move(callback)));
    }
}

void FrameCodec::UnregisterTransformedFrameCallback()
{
    _impl->_sink({});
}

void FrameCodec::UnregisterTransformedFrameSinkCallback(uint32_t ssrc)
{
    LOCK_WRITE_SAFE_OBJ(_impl->_sinks);
    _impl->_sinks->erase(ssrc);
}

webrtc::scoped_refptr<FrameCodec> FrameCodec::
    create(cricket::MediaType mediaType,
           std::string trackId,
           const std::weak_ptr<rtc::Thread>& signalingThread,
           const std::shared_ptr<KeyProvider>& keyProvider,
           const std::shared_ptr<Bricks::Logger>& logger)
{
    if (!keyProvider) {
        logInitError(logger, "no key provider");
        return {};
    }
    switch (mediaType) {
        case cricket::MEDIA_TYPE_VIDEO:
        case cricket::MEDIA_TYPE_AUDIO:
            break;
        default:
            logInitError(logger, "unsupported media type: " + cricket::MediaTypeToString(mediaType));
            return {};
    }
    if (trackId.empty()) {
        logInitError(logger, "unknown track ID");
        return {};
    }
    return webrtc::make_ref_counted<FrameCodec>(mediaType, std::move(trackId),
                                                signalingThread,
                                                keyProvider, logger);
}

FrameCodec::Impl::Impl(cricket::MediaType mediaType,
                       std::string trackId,
                       const std::weak_ptr<rtc::Thread>& signalingThread,
                       const std::shared_ptr<KeyProvider>& keyProvider,
                       const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _mediaType(mediaType)
    , _trackId(std::move(trackId))
    , _keyProvider(keyProvider)
    , _logCategory(std::string(g_category) + "_" + _trackId)
    , _observer(signalingThread)
{
}

bool FrameCodec::Impl::hasSink() const
{
    LOCK_READ_SAFE_OBJ(_sink);
    return nullptr != _sink->get();
}

bool FrameCodec::Impl::hasSinks() const
{
    LOCK_READ_SAFE_OBJ(_sinks);
    return !_sinks->empty();
}

void FrameCodec::Impl::transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame) {
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

void FrameCodec::Impl::encryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame) {
        webrtc::scoped_refptr<webrtc::TransformedFrameCallback> sink;
        if (cricket::MEDIA_TYPE_AUDIO == _mediaType) {
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
            logWarning("unable to encrypt frame, sink is NULL");
            setLastEncryptState(FrameCodecState::InternalError);
            return;
        }
        
        const auto dataIn = frame->GetData();
        if (dataIn.empty()) {
            logWarning("no input data for encrypt");
            return;
        }
        if (!_enabledCryption) {
            logWarning("encryption disabled");
            if (_keyProvider->options()._discardFrameWhenCryptorNotReady) {
                return;
            }
            sink->OnTransformedFrame(std::move(frame));
            return;
        }
        
        const auto keyHandler = this->keyHandler();
        if (!keyHandler) {
            logError("no keys for participant");
            setLastEncryptState(FrameCodecState::MissingKey);
            return;
        }
        
        const auto keyIndex = _keyIndex.load();
        const auto keySet = keyHandler->keySet(keyIndex);
        if (!keySet) {
            logError("no key set for key index " + std::to_string(keyIndex));
            setLastEncryptState(FrameCodecState::MissingKey);
            return;
        }
        
        const auto frameHeaderSize = unencryptedBytes(frame.get(), _mediaType);
        rtc::Buffer frameHeader(frameHeaderSize);
        for (size_t i = 0U; i < frameHeaderSize; i++) {
            frameHeader[i] = dataIn[i];
        }
        
        rtc::Buffer iv = makeIv(frame->GetSsrc(), frame->GetTimestamp());
        
        rtc::Buffer payload(dataIn.size() - frameHeaderSize);
        for (size_t i = frameHeaderSize; i < dataIn.size(); i++) {
            payload[i - frameHeaderSize] = dataIn[i];
        }
        
        std::vector<uint8_t> buffer;
        if (aesGcmEncryptDecrypt(true, keySet->_encryptionKey, payload, iv,
                                 frameHeader, buffer)) {
            rtc::Buffer frameTrailer(2U);
            frameTrailer[0] = ivSize();
            frameTrailer[1] = _keyIndex;
            
            rtc::Buffer encryptedPayload(buffer.data(), buffer.size());
            rtc::Buffer tag(encryptedPayload.data() + encryptedPayload.size() - 16, 16);
            
            rtc::Buffer dataWithoutHeader;
            dataWithoutHeader.AppendData(encryptedPayload);
            dataWithoutHeader.AppendData(iv);
            dataWithoutHeader.AppendData(frameTrailer);

            rtc::Buffer dataOut;
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
            setLastEncryptState(FrameCodecState::Ok);
            sink->OnTransformedFrame(std::move(frame));
        }
        else {
            setLastEncryptState(FrameCodecState::EncryptionFailed);
            logError("encrypt frame failed");
        }
    }
}

void FrameCodec::Impl::decryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame)
{
    if (frame) {
        webrtc::scoped_refptr<webrtc::TransformedFrameCallback> sink;
        if (cricket::MEDIA_TYPE_AUDIO == _mediaType) {
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
            logWarning("unable to decrypt frame, sink is NULL");
            setLastDecryptState(FrameCodecState::InternalError);
            return;
        }
        
        const auto dataIn = frame->GetData();
        if (dataIn.empty() || !_enabledCryption) {
            logWarning("no input data for decrypt or decryption disabled");
            if (_keyProvider->options()._discardFrameWhenCryptorNotReady) {
                return;
            }
            sink->OnTransformedFrame(std::move(frame));
            return;
        }
    }
}

void FrameCodec::Impl::setLastEncryptState(FrameCodecState state)
{
    if (exchangeVal(state, _lastEncState) && _observer) {
        _observer.invoke(&FrameCodecObserver::onEncryptionStateChanged,
                         _trackId, state);
    }
}

void FrameCodec::Impl::setLastDecryptState(FrameCodecState state)
{
    if (exchangeVal(state, _lastDecState) && _observer) {
        _observer.invoke(&FrameCodecObserver::onDecryptionStateChanged,
                         _trackId, state);
    }
}

std::shared_ptr<ParticipantKeyHandler> FrameCodec::Impl::keyHandler() const
{
    if (_keyProvider->options()._sharedKey) {
        return _keyProvider->sharedKey(_trackId);
    }
    return _keyProvider->key(_trackId);
}

bool FrameCodec::Impl::aesGcmEncryptDecrypt(bool encrypt,
                                            const std::vector<uint8_t>& rawKey,
                                            const rtc::ArrayView<uint8_t>& data,
                                            const rtc::ArrayView<uint8_t>& iv,
                                            const rtc::ArrayView<uint8_t>& additionalData,
                                            std::vector<uint8_t>& buffer) const
{
    const EVP_AEAD* aeadAlg = aesGcmAlgorithmFromKeySize(rawKey.size());
    if (!aeadAlg) {
        logError("invalid AES-GCM key size");
        return false;
    }
    static constexpr unsigned tagLengthBytes = 128U / 8U;
    
    bssl::ScopedEVP_AEAD_CTX ctx;
    if (!EVP_AEAD_CTX_init(ctx.get(), aeadAlg,
                           rawKey.data(), rawKey.size(),
                           tagLengthBytes, nullptr)) {
        logError("failed to initialize AES-GCM context");
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
            logError("data too small for AES-GCM tag");
            return false;
        }
        buffer.resize(data.size() - tagLengthBytes);
        ok = EVP_AEAD_CTX_open(ctx.get(), buffer.data(), &len, buffer.size(),
                               iv.data(), iv.size(), data.data(), data.size(),
                               additionalData.data(), additionalData.size());
    }
    if (!ok) {
        logWarning("failed to perform AES-GCM operation");
        return false;
    }

    buffer.resize(len);
    return true;
}

rtc::Buffer FrameCodec::Impl::makeIv(uint32_t ssrc, uint32_t timestamp)
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
    rtc::ByteBufferWriter buf;
    buf.WriteUInt32(ssrc);
    buf.WriteUInt32(timestamp);
    buf.WriteUInt32(timestamp - (sendCount % 0xFFFF));
    _sendCounts[ssrc] = sendCount + 1;
    RTC_CHECK_EQ(buf.Length(), ivSize());
    return rtc::Buffer(buf.Data(), buf.Length());
}

} // namespace LiveKitCpp


namespace
{

uint8_t unencryptedH264Bytes(const rtc::ArrayView<const uint8_t>& data)
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

uint8_t unencryptedH265Bytes(const rtc::ArrayView<const uint8_t>& data)
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

uint8_t unencryptedBytes(const webrtc::TransformableFrameInterface* frame, cricket::MediaType type)
{
    if (frame) {
        if (cricket::MEDIA_TYPE_AUDIO == type) {
            return 1; // opus
        }
        if (cricket::MEDIA_TYPE_VIDEO == type) {
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
                case webrtc::kVideoCodecVP9:
                    if (webrtc::VideoFrameType::kVideoFrameKey == metaData.GetFrameType()) {
                        return 10;
                    }
                    // 1 byte is mandatory + Picture ID (1-2 bytes, opt.) + TL0PICIDX (opt.)
                    return 5;
                case webrtc::kVideoCodecAV1:
                    return 5; // OBU header
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
                                cricket::MediaType mediaType,
                                webrtc::VideoCodecType codecType)
{
    if (frame && cricket::MEDIA_TYPE_VIDEO == mediaType) {
        const auto videoFrame = static_cast<const webrtc::TransformableVideoFrameInterface*>(frame);
        return codecType == videoFrame->Metadata().GetCodec();
    }
    return false;
}

bool frameIsH264(const webrtc::TransformableFrameInterface* frame, cricket::MediaType type)
{
    return checkVideoCodecType(frame, type, webrtc::kVideoCodecH264);
}

bool frameIsH265(const webrtc::TransformableFrameInterface* frame, cricket::MediaType type)
{
    return checkVideoCodecType(frame, type, webrtc::kVideoCodecH265);
}

}
