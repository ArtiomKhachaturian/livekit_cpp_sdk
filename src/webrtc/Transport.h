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
#pragma once // Transport.h
#include "Loggable.h"
#include "rtc/ClientConfiguration.h"
#include "rtc/SignalTarget.h"
#include "rtc/ICEServer.h"
#include <api/peer_connection_interface.h>
#include <memory>
#include <optional>
#include <vector>

namespace webrtc {
class PeerConnectionObserver;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;

class Transport : private LoggableShared<>
{
public:
    static std::unique_ptr<Transport> create(bool primary, SignalTarget target,
                                             webrtc::PeerConnectionObserver* observer,
                                             const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                                             const webrtc::PeerConnectionInterface::RTCConfiguration& conf);
private:
    Transport(bool primary, SignalTarget target,
              const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
              webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc);
private:
    const bool _primary;
    const SignalTarget _target;
    const webrtc::scoped_refptr<webrtc::RefCountInterface> _; // maintain reference
    const webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
    
};

} // namespace LiveKitCpp
