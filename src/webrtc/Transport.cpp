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
#include "Transport.h"
#include "PeerConnectionFactory.h"

namespace LiveKitCpp
{

Transport::Transport(bool primary, SignalTarget target,
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc)
    : LoggableShared<>(pcf->logger())
    , _primary(primary)
    , _target(target)
    , _(pcf)
    , _pc(std::move(pc))
{
}

std::unique_ptr<Transport> Transport::create(bool primary, SignalTarget target,
                                             webrtc::PeerConnectionObserver* observer,
                                             const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                             const webrtc::PeerConnectionInterface::RTCConfiguration& conf)
{
    std::unique_ptr<Transport> transport;
    if (observer && pcf) {
        auto pc = pcf->CreatePeerConnectionOrError(conf, webrtc::PeerConnectionDependencies(observer));
        if (pc.ok()) {
            transport.reset(new Transport(primary, target, pcf, std::move(pc.MoveValue())));
        }
        else if (const auto logger = pcf->logger()) {
            logger->logError(pc.error().message(), "CreatePeerConnection");
        }
    }
    return transport;
}

} // namespace LiveKitCpp
