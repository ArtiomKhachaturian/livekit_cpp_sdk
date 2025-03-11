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
#include "Options.h"
#include "LocalAudioTrackPromise.h"
#include "SafeScopedRefPtr.h"
#include "SignalServerListener.h"
#include "DataChannelListener.h"
#include "SignalClientWs.h"
#include "LocalTrackFactory.h"
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
class RtpSenderInterface;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;
class TransportManager;
class JoinResponse;

// https://github.com/livekit/client-sdk-js/blob/main/src/room/RTCEngine.ts
class RTCEngine : private Bricks::LoggableS<TransportManagerListener,
                                            SignalTransportListener,
                                            SignalServerListener,
                                            DataChannelListener,
                                            LocalTrackFactory>
{
    using SafeString = Bricks::SafeObj<std::string>;
public:
    RTCEngine(const Options& signalOptions,
              PeerConnectionFactory* pcf,
              std::unique_ptr<Websocket::EndPoint> socket,
              const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCEngine() final;
    bool connect(std::string url, std::string authToken);
    void setMicrophoneEnabled(bool enable);
private:
    void cleanup(bool /*error*/ = false);
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const std::vector<ICEServer>& iceServers = {},
                          const std::optional<ClientConfiguration>& cc = {}) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const JoinResponse& response) const;
    webrtc::PeerConnectionInterface::RTCConfiguration
        makeConfiguration(const ReconnectResponse& response) const;
    // impl. of TransportManagerListener
    void onNegotiationNeeded() final;
    void onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel) final;
    void onPublisherOffer(const webrtc::SessionDescriptionInterface* desc) final;
    void onSubscriberAnswer(const webrtc::SessionDescriptionInterface* desc) final;
    void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender) final;
    void onLocalTrackAddFailure(const std::string& id,
                                cricket::MediaType type,
                                const std::vector<std::string>&,
                                webrtc::RTCError) final;
    void onIceCandidateGathered(SignalTarget target,
                                const webrtc::IceCandidateInterface* candidate) final;
    // impl. of SignalServerListener
    void onJoin(uint64_t, const JoinResponse& response) final;
    void onOffer(uint64_t, const SessionDescription& sdp) final;
    void onAnswer(uint64_t, const SessionDescription& sdp) final;
    void onPong(uint64_t, int64_t, int64_t) final;
    void onTrickle(uint64_t, const TrickleRequest& request) final;
    // impl. of SignalTransportListener
    void onTransportStateChanged(uint64_t, TransportState state) final;
    void onTransportError(uint64_t, std::string error) final;
    // impl. of PingPongKitListener
    bool onPingRequested() final;
    void onPongTimeout() final;
    // impl. of DataChannelListener
    void onStateChange(DataChannel* channel) final;
    void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) final;
    void onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize) final;
    // impl. LocalTrackFactory
    bool add(webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track) final;
    bool remove(webrtc::scoped_refptr<webrtc::RtpSenderInterface> sender) final;
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createAudio(const std::string& label,
                                                                   const cricket::AudioOptions& options) final;
    // impl. of Bricks::LoggableS<>
    std::string_view logCategory() const final;
private:
    const Options _options;
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    SignalClientWs _client;
    std::shared_ptr<TransportManager> _pcManager;
    SafeScopedRefPtr<DataChannel> _lossyDC;
    SafeScopedRefPtr<DataChannel> _reliableDC;
    LocalAudioTrackPromise _microphone;
    /** keeps track of how often an initial join connection has been tried */
    std::atomic_uint _joinAttempts = 0U;
};

} // namespace LiveKitCpp
