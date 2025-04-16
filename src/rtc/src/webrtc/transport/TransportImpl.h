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
#pragma once // TransportImpl.h
#include "Listener.h"
#include "Loggable.h"
#include <api/peer_connection_interface.h>
#include <rtc_base/thread.h>
#include <atomic>

namespace LiveKitCpp
{

enum class SignalTarget;
class TransportListener;
class PeerConnectionFactory;

class TransportImpl : public Bricks::LoggableS<webrtc::PeerConnectionObserver>,
                      public std::enable_shared_from_this<TransportImpl>
{
public:
    TransportImpl(SignalTarget target,
                  const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                  TransportListener* listener,
                  const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                  const std::string& identity,
                  const std::shared_ptr<Bricks::Logger>& logger);
    bool closed() const noexcept { return _closed; }
    SignalTarget target() const { return _target; }
    std::shared_ptr<rtc::Thread> signalingThread() const;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> peerConnection() const noexcept;
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const {
        _listener.invoke(method, _target, std::forward<Args>(args)...);
    }
    void close();
    void logWebRTCError(const webrtc::RTCError& error, std::string_view prefix = {}) const;
    void removeTrackBySender(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    webrtc::PeerConnectionInterface::IceConnectionState iceConnectionState() const noexcept;
    webrtc::PeerConnectionInterface::SignalingState signalingState() const noexcept;
    webrtc::PeerConnectionInterface::IceGatheringState iceGatheringState() const noexcept;
    // impl. of webrtc::PeerConnectionObserver
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) final;
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) final;
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) final;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) final;
    void OnNegotiationNeededEvent(uint32_t eventId) final;
    void OnStandardizedIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState newState) final;
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState newState) final;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState newState) final;
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) final;
    void OnIceCandidateError(const std::string& address, int port, const std::string& url,
                             int errorCode, const std::string& errorText) final;
    void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) final;
    void OnIceConnectionReceivingChange(bool receiving) final;
    void OnIceSelectedCandidatePairChanged(const cricket::CandidatePairChangeEvent& event) final;
    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) final;
    //void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    void OnInterestingUsage(int usagePattern) final;
protected:
    // overrides of Bricks::LoggableS
    std::string_view logCategory() const final { return _logCategory; }
private:
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface>
    createPeerConnection(const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                         webrtc::PeerConnectionObserver* observer) const;
    template <typename TState>
    bool changeAndLogState(TState newState, std::atomic<TState>& val) const;
private:
    const SignalTarget _target;
    const std::string _logCategory;
    const std::weak_ptr<rtc::Thread> _signalingThread;
    Bricks::Listener<TransportListener*> _listener;
    std::atomic_bool _closed = false;
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _pcState;
    std::atomic<webrtc::PeerConnectionInterface::IceConnectionState> _iceConnState;
    std::atomic<webrtc::PeerConnectionInterface::SignalingState> _signalingState;
    std::atomic<webrtc::PeerConnectionInterface::IceGatheringState> _iceGatheringState;
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> _pc;
};
	
} // namespace LiveKitCpp
