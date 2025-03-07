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
#pragma once // Engine.h
#include "Loggable.h"
#include "SignalOptions.h"
#include "SafeObjAliases.h"
#include "SafeScopedRefPtr.h"
#include "SignalServerListener.h"
#include "SignalClientWs.h"
#include "TransportManagerListener.h"
#include "rtc/ClientConfiguration.h"
#include "rtc/JoinResponse.h"
#include <api/scoped_refptr.h>
#include <atomic>

namespace Websocket {
class EndPoint;
}

namespace webrtc {
class DataChannelInterface;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;
class TransportManager;
class JoinResponse;

// https://github.com/livekit/client-sdk-js/blob/main/src/room/RTCEngine.ts
class RTCEngine : private Bricks::LoggableS<TransportManagerListener, SignalServerListener>
{
public:
    RTCEngine(const SignalOptions& signalOptions,
              const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
              std::unique_ptr<Websocket::EndPoint> socket);
    ~RTCEngine() final;
private:
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const std::vector<ICEServer>& iceServers = {},
                          const std::optional<ClientConfiguration>& cc = {}) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const JoinResponse& response) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const ReconnectResponse& response) const;
    // impl. of Bricks::LoggableR<>
    std::string_view logCategory() const final;
private:
    const SignalOptions _signalOptions;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    SignalClientWs _client;
    // peerConnectionTimeout: number = roomConnectOptionDefaults.peerConnectionTimeout;
    std::atomic_bool _fullReconnectOnNext = false;
    Bricks::SafeUniquePtr<TransportManager> _pcManager;
    Bricks::SafeOptional<JoinResponse> _latestJoinResponse;
    SafeScopedRefPtr<webrtc::DataChannelInterface> _lossyDC;
    SafeScopedRefPtr<webrtc::DataChannelInterface> _lossyDCSub;
    SafeScopedRefPtr<webrtc::DataChannelInterface> _reliableDC;
    // private dcBufferStatus: Map<DataPacket_Kind, boolean>;
    SafeScopedRefPtr<webrtc::DataChannelInterface> _reliableDCSub;
    std::atomic_bool _subscriberPrimary = false;
    // private pcState: PCState = PCState.New;
    std::atomic_bool _isClosed = true;
    // keep join info around for reconnect, this could be a region url
    /*
      private url?: string;

      private token?: string;*/
    std::atomic_uint _reconnectStart = 0U;
    Bricks::SafeOptional<ClientConfiguration> _clientConfiguration;
    std::atomic_bool _attemptingReconnect = false;
    // private reconnectPolicy: ReconnectPolicy;
    // private reconnectTimeout?: ReturnType<typeof setTimeout>;
    Bricks::SafeObj<std::string> _participantSid;
    /** keeps track of how often an initial join connection has been tried */
    std::atomic_uint _joinAttempts = 0U;
    /** specifies how often an initial join connection is allowed to retry */
    std::atomic_uint _maxJoinAttempts = 1U;
};

} // namespace LiveKitCpp
