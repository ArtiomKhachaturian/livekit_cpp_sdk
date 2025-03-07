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
#include "SignalTransportListener.h"
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
class RTCEngine : private Bricks::LoggableS<TransportManagerListener,
                                            SignalTransportListener,
                                            SignalServerListener>
{
    using SafeString = Bricks::SafeObj<std::string>;
public:
    RTCEngine(const SignalOptions& signalOptions,
              PeerConnectionFactory* pcf,
              std::unique_ptr<Websocket::EndPoint> socket);
    ~RTCEngine() final;
    bool connect(std::string url, std::string authToken);
private:
    void negotiate();
    void createDataChannels(TransportManager* pcManager);
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const std::vector<ICEServer>& iceServers = {},
                          const std::optional<ClientConfiguration>& cc = {}) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const JoinResponse& response) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const ReconnectResponse& response) const;
    // impl. of TransportManagerListener
    void onPublisherOffer(TransportManager&,
                          const webrtc::SessionDescriptionInterface* desc) final;
    void onSubscriberAnswer(TransportManager& manager,
                            const webrtc::SessionDescriptionInterface* desc) final;
    // impl. of SignalServerListener
    void onJoin(uint64_t, const JoinResponse& response) final;
    void onOffer(uint64_t, const SessionDescription& sdp) final;
    void onAnswer(uint64_t, const SessionDescription& sdp) final;
    
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
    std::atomic_uint _reconnectStart = 0U;
    std::atomic_bool _attemptingReconnect = false;
    // private reconnectPolicy: ReconnectPolicy;
    // private reconnectTimeout?: ReturnType<typeof setTimeout>;
    /** keeps track of how often an initial join connection has been tried */
    std::atomic_uint _joinAttempts = 0U;
    /** specifies how often an initial join connection is allowed to retry */
    std::atomic_uint _maxJoinAttempts = 1U;
};

} // namespace LiveKitCpp
