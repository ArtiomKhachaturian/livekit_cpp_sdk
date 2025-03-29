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
#pragma once // AsyncMicSourceImpl.h
#include "AsyncAudioSourceImpl.h"
#include <memory>

namespace LiveKitCpp
{

class AdmProxyFacade;

class AsyncMicSourceImpl : public AsyncAudioSourceImpl
{
public:
    AsyncMicSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                       const std::shared_ptr<Bricks::Logger>& logger,
                       std::shared_ptr<AdmProxyFacade> admProxy);
    ~AsyncMicSourceImpl() override;
    // impl. of AsyncAudioSourceImpl
    void addSink(webrtc::AudioTrackSinkInterface* sink) final;
    void removeSink(webrtc::AudioTrackSinkInterface* sink) final;
    cricket::AudioOptions options() const final;
private:
    const std::shared_ptr<AdmProxyFacade> _admProxy;
};

} // namespace LiveKitCpp
