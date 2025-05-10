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
#pragma once // TransportManager.h
#include "RtcObject.h"
#include <api/peer_connection_interface.h>

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class TrackManager;
class TransportManagerImpl;
class TransportManagerListener;
class PeerConnectionFactory;
class DataChannelObserver;
class AudioDeviceImpl;
class LocalVideoDeviceImpl;
enum class SignalTarget;
enum class EncryptionType;

class TransportManager : private RtcObject<TransportManagerImpl>
{
public:
    TransportManager(bool subscriberPrimary, bool fastPublish,
                     int32_t pingTimeout, int32_t pingInterval,
                     uint64_t negotiationDelay, // ms
                     const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
                     const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
                     const std::weak_ptr<TrackManager>& trackManager,
                     const std::string& identity,
                     const std::string& prefferedAudioEncoder = {},
                     const std::string& prefferedVideoEncoder = {},
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~TransportManager();
    bool valid() const noexcept;
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    bool closed() const noexcept;
    void negotiate(bool force = false);
    void startPing();
    void stopPing();
    void notifyThatPongReceived();
    bool setRemoteOffer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    bool setRemoteAnswer(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void addTrack(std::shared_ptr<AudioDeviceImpl> device, EncryptionType encryption);
    void addTrack(std::shared_ptr<LocalVideoDeviceImpl> device, EncryptionType encryption);
    bool removeTrack(const rtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track);
    void addIceCandidate(SignalTarget target, std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    void queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void queryStats(const rtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver,
                    const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void queryStats(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender,
                    const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    void setAudioPlayout(bool playout);
    void setAudioRecording(bool recording);
    void close();
    void setListener(TransportManagerListener* listener);
};

} // namespace LiveKitCpp
