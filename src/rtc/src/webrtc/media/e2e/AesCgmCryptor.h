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
#pragma once // AesCgmCryptor.h
#include "AesCgmCryptorState.h"
#include "AsyncListener.h"
#include "Loggable.h"
#include "SafeScopedRefPtr.h"
#include <api/media_types.h>
#include <api/frame_transformer_interface.h>
#include <api/task_queue/task_queue_base.h>
#include <atomic>
#include <map>
#include <memory>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AesCgmCryptorObserver;
class E2EKeyHandler;
class KeyProvider;

// AES GGM codec
class AesCgmCryptor : public Bricks::LoggableS<webrtc::FrameTransformerInterface>
{
    using Sinks = std::map<uint32_t, webrtc::scoped_refptr<webrtc::TransformedFrameCallback>>;
public:
    ~AesCgmCryptor() override;
    static webrtc::scoped_refptr<AesCgmCryptor> create(webrtc::MediaType mediaType,
                                                       std::string identity,
                                                       std::string trackId,
                                                       std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                                       const std::shared_ptr<KeyProvider>& keyProvider,
                                                       const std::shared_ptr<Bricks::Logger>& logger = {});
    void setKeyIndex(uint8_t keyIndex) { _keyIndex = keyIndex; }
    uint8_t keyIndex() const { return _keyIndex; }
    void setEnabled(bool enabled) { _enabledCryption = enabled; }
    bool enabled() const { return _enabledCryption; }
    void setObserver(const std::weak_ptr<AesCgmCryptorObserver>& observer = {}) { _observer = observer; }
    // impl. of webrtc::FrameTransformerInterface
    void Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame) final;
    void RegisterTransformedFrameCallback(rtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) final;
    void RegisterTransformedFrameSinkCallback(rtc::scoped_refptr<webrtc::TransformedFrameCallback> callback,
                                              uint32_t ssrc) final;
    void UnregisterTransformedFrameCallback() final;
    void UnregisterTransformedFrameSinkCallback(uint32_t ssrc) final;
protected:
    AesCgmCryptor(webrtc::MediaType mediaType,
                  std::string identity,
                  std::string trackId,
                  std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                  const std::shared_ptr<KeyProvider>& keyProvider,
                  const std::shared_ptr<Bricks::Logger>& logger);
    // override of Bricks::LoggableS<>
    std::string_view logCategory() const final { return _logCategory; }
private:
    static constexpr uint8_t ivSize() noexcept { return 12; }
    bool hasSink() const;
    bool hasSinks() const;
    void encryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame);
    void decryptFrame(std::unique_ptr<webrtc::TransformableFrameInterface> frame);
    void setLastEncryptState(AesCgmCryptorState state,
                             const std::string& comment = {},
                             Bricks::LoggingSeverity severity = Bricks::LoggingSeverity::Verbose);
    void setLastDecryptState(AesCgmCryptorState state,
                             const std::string& comment = {},
                             Bricks::LoggingSeverity severity = Bricks::LoggingSeverity::Verbose);
    std::shared_ptr<E2EKeyHandler> keyHandler() const;
    bool encryptOrDecrypt(bool encrypt,
                          const std::vector<uint8_t>& rawKey,
                          const webrtc::ArrayView<uint8_t>& iv,
                          const webrtc::ArrayView<uint8_t>& additionalData,
                          const webrtc::ArrayView<uint8_t>& data,
                          std::vector<uint8_t>& buffer) const;
    rtc::Buffer makeIv(uint32_t ssrc, uint32_t timestamp);
private:
    static thread_local inline std::map<uint32_t, uint32_t> _sendCounts;
    const webrtc::MediaType _mediaType;
    const std::string _identity;
    const std::string _trackId;
    const std::shared_ptr<KeyProvider> _keyProvider;
    const std::string _logCategory;
    std::atomic<uint8_t> _keyIndex = 0;
    std::atomic_bool _enabledCryption = true;
    AsyncListener<std::weak_ptr<AesCgmCryptorObserver>, true> _observer;
    SafeScopedRefPtr<webrtc::TransformedFrameCallback> _sink;
    Bricks::SafeObj<Sinks> _sinks;
    std::atomic<AesCgmCryptorState> _lastEncState = AesCgmCryptorState::New;
    std::atomic<AesCgmCryptorState> _lastDecState = AesCgmCryptorState::New;
};

} // namespace LiveKitCpp
