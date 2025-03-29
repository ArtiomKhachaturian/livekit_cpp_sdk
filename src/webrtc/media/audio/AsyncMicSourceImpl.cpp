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
#include "AsyncMicSourceImpl.h"
#include "AdmProxyFacade.h"
#include "AudioSinks.h"

namespace LiveKitCpp
{

AsyncMicSourceImpl::AsyncMicSourceImpl(std::weak_ptr<webrtc::TaskQueueBase> signalingQueue,
                                       const std::shared_ptr<Bricks::Logger>& logger,
                                       std::shared_ptr<AdmProxyFacade> admProxy)
    : AsyncAudioSourceImpl(std::move(signalingQueue), logger, true)
    , _admProxy(std::move(admProxy))
{
}

AsyncMicSourceImpl::~AsyncMicSourceImpl()
{
}

void AsyncMicSourceImpl::addSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (dynamic_cast<AudioSinks*>(sink)) {
        AsyncAudioSourceImpl::addSink(sink);
    }
}

void AsyncMicSourceImpl::removeSink(webrtc::AudioTrackSinkInterface* sink)
{
    if (dynamic_cast<AudioSinks*>(sink)) {
        AsyncAudioSourceImpl::removeSink(sink);
    }
}

cricket::AudioOptions AsyncMicSourceImpl::options() const
{
    if (_admProxy) {
        return _admProxy->options();
    }
    return {};
}

} // namespace LiveKitCpp
