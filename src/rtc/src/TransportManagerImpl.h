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
#pragma once // TransportManagerImpl.h
#include "Loggable.h"
#include "AsyncListener.h"
#include "PingPongKit.h"
#include "PingPongKitListener.h"
#include "SafeObj.h"
#include "Transport.h"
#include "TransportListener.h"
#include "livekit/signaling/sfu/TrackInfo.h"
#include <api/peer_connection_interface.h>
#include <vector>
#include <unordered_map>

namespace LiveKitCpp
{

class LocalTrackAccessor;
class LocalAudioTrackImpl;
class LocalVideoTrackImpl;
class TransportManagerListener;
class TrackManager;

class TransportManagerImpl : private Bricks::LoggableS<TransportListener, PingPongKitListener>
{
    template <class T> using Tracks = std::unordered_map<std::string, std::shared_ptr<T>>;
public:
    TransportManagerImpl(bool subscriberPrimary,
                         bool fastPublish,
                         bool disableAudioRed,
                         int32_t pingTimeout,
                         int32_t pingInterval,
                         uint64_t negotiationDelay,
                         std::vector<TrackInfo> tracksInfo,
                         const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                         const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                         const std::weak_ptr<TrackManager>& trackManager,
                         const std::string& identity,
                         const std::string& prefferedAudioEncoder = {},
                         const std::string& prefferedVideoEncoder = {},
                         const std::shared_ptr<Bricks::Logger>& logger = {});
    ~TransportManagerImpl() final;
    bool valid() const noexcept;
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    bool closed() const noexcept;
    void negotiate(bool force = false);
    void startPing() { _pingPongKit.start(this); }
    void stopPing() { _pingPongKit.stop(); }
    void notifyThatPongReceived() { _pingPongKit.notifyThatPongReceived(); }
    bool setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void addTrack(std::shared_ptr<AudioDeviceImpl> device, EncryptionType encryption);
    void addTrack(std::shared_ptr<LocalVideoDeviceImpl> device, EncryptionType encryption);
    bool removeTrack(const std::string& id, bool cid = true);
    void addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    void queryStats(const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void queryReceiverStats(const webrtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                            const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void querySenderStats(const webrtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                          const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void setAudioPlayout(bool playout);
    void setAudioRecording(bool recording);
    void close();
    void setListener(TransportManagerListener* listener);
    void updateTracksInfo(std::vector<TrackInfo> tracksInfo);
    bool setRemoteSideTrackMute(const std::string& trackSid, bool mute);
    std::shared_ptr<LocalTrackAccessor> track(const std::string& id, bool cid,
                                              const std::optional<webrtc::MediaType>& hint = std::nullopt) const;
private:
    template <class TTracks>
    static std::shared_ptr<LocalTrackAccessor> lookup(const std::string& id, bool cid, const TTracks& tracks);
    template <webrtc::MediaType type, class TTracks>
    void remove(const std::string& id, TTracks& tracks) const;
    template <webrtc::MediaType type, class TTracks>
    void remove(TTracks& tracks) const;
    void createPublisherOffer();
    bool canNegotiate() const noexcept;
    bool localDataChannelsAreCreated() const noexcept { return _embeddedDCMaxCount == _embeddedDCCount; }
    Transport& primaryTransport() noexcept;
    const Transport& primaryTransport() const noexcept;
    bool isPrimary(SignalTarget target) const noexcept;
    void updateState();
    void cancelNegotiationTimer();
    // impl. of TransportListener
    void onSdpCreated(SignalTarget target, std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onSdpCreationFailure(SignalTarget target, webrtc::SdpType type, webrtc::RTCError error) final;
    void onSdpSet(SignalTarget target, bool local, const webrtc::SessionDescriptionInterface* desc) final;
    void onSdpSetFailure(SignalTarget target, bool local, webrtc::RTCError error) final;
    void onLocalAudioTrackAdded(SignalTarget target, std::shared_ptr<AudioDeviceImpl> device,
                                EncryptionType encryption,
                                webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void onLocalVideoTrackAdded(SignalTarget target, std::shared_ptr<LocalVideoDeviceImpl> device,
                                EncryptionType encryption,
                                webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) final;
    void onLocalTrackAddFailure(SignalTarget target, const std::string& id, webrtc::MediaType type,
                                const webrtc::RtpTransceiverInit&, webrtc::RTCError error) final;
    void onLocalTrackRemoved(SignalTarget target, const std::string& id, webrtc::MediaType type,
                             const std::vector<std::string>&) final;
    void onLocalDataChannelCreated(SignalTarget target,
                                   webrtc::scoped_refptr<DataChannel> channel) final;
    void onConnectionChange(SignalTarget, webrtc::PeerConnectionInterface::PeerConnectionState) final;
    void onIceConnectionChange(SignalTarget, webrtc::PeerConnectionInterface::IceConnectionState) final;
    void onSignalingChange(SignalTarget, webrtc::PeerConnectionInterface::SignalingState) final;
    void onRemoteDataChannelOpened(SignalTarget target,
                                   webrtc::scoped_refptr<DataChannel> channel) final;
    void onIceCandidateGathered(SignalTarget target, const webrtc::IceCandidateInterface* candidate) final;
    void onRemoteTrackAdded(SignalTarget target, webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            const std::vector<webrtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) final;
    void onRemotedTrackRemoved(SignalTarget target,
                               webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) final;
    // impl. of PingPongKitListener
    bool onPingRequested() final;
    void onPongTimeout() final;
    // override of Bricks::LoggableS<>
    std::string_view logCategory() const final { return _logCategory; }
private:
    static constexpr uint8_t _embeddedDCMaxCount = 2U;
    const uint64_t _negotiationTimerId;
    const uint64_t _negotiationDelay;
    const bool _subscriberPrimary;
    const bool _fastPublish;
    const bool _disableAudioRed;
    const std::string _logCategory;
    const std::weak_ptr<TrackManager> _trackManager;
    AsyncListener<TransportManagerListener*> _listener;
    MediaTimer _negotiationTimer;
    Transport _publisher;
    Transport _subscriber;
    PingPongKit _pingPongKit;
    std::atomic<webrtc::PeerConnectionInterface::PeerConnectionState> _state;
    std::atomic<uint8_t> _embeddedDCCount = 0U;
    std::atomic_bool _pendingNegotiation = false;
    std::atomic_bool _embeddedDCRequested = false;
    Bricks::SafeObj<std::vector<TrackInfo>> _tracksInfo;
    Bricks::SafeObj<Tracks<LocalAudioTrackImpl>> _audioTracks;
    Bricks::SafeObj<Tracks<LocalVideoTrackImpl>> _videoTracks;
};
	
} // namespace LiveKitCpp
