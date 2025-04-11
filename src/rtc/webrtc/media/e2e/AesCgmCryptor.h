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
#include <api/media_types.h>
#include <api/frame_transformer_interface.h>
#include <api/task_queue/task_queue_base.h>
#include <memory>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AesCgmCryptorObserver;
class KeyProvider;

// AES GGM codec
class AesCgmCryptor : public webrtc::FrameTransformerInterface
{
    
    struct Impl;
public:
    ~AesCgmCryptor() override;
    static webrtc::scoped_refptr<AesCgmCryptor> create(cricket::MediaType mediaType,
                                                       std::string identity,
                                                       std::string trackId,
                                                       std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                                       const std::shared_ptr<KeyProvider>& keyProvider,
                                                       const std::shared_ptr<Bricks::Logger>& logger = {});
    void setKeyIndex(uint8_t keyIndex);
    uint8_t keyIndex() const;
    void setEnabled(bool enabled);
    bool enabled() const;
    void setObserver(const std::weak_ptr<AesCgmCryptorObserver>& observer = {});
    // impl. of webrtc::FrameTransformerInterface
    void Transform(std::unique_ptr<webrtc::TransformableFrameInterface> frame) final;
    void RegisterTransformedFrameCallback(rtc::scoped_refptr<webrtc::TransformedFrameCallback> callback) final;
    void RegisterTransformedFrameSinkCallback(rtc::scoped_refptr<webrtc::TransformedFrameCallback> callback,
                                              uint32_t ssrc) final;
    void UnregisterTransformedFrameCallback() final;
    void UnregisterTransformedFrameSinkCallback(uint32_t ssrc) final;
protected:
    AesCgmCryptor(cricket::MediaType mediaType,
                  std::string identity,
                  std::string trackId,
                  std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                  const std::shared_ptr<KeyProvider>& keyProvider,
                  const std::shared_ptr<Bricks::Logger>& logger);
private:
    const std::shared_ptr<Impl> _impl;
    const std::unique_ptr<webrtc::TaskQueueBase, webrtc::TaskQueueDeleter> _queue;
};

} // namespace LiveKitCpp
