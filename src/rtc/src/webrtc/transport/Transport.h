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
#include "CreateSdpListener.h"
#include "SetSdpListener.h"
#include "RtcObject.h"
#include "livekit/signaling/sfu/SignalTarget.h"
#include <api/peer_connection_interface.h>
#include <atomic>
#include <memory>
#include <optional>
#include <vector>

namespace webrtc {
class PeerConnectionObserver;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class PeerConnectionFactory;
class CreateSdpObserver;
class SetLocalSdpObserver;
class SetRemoteSdpObserver;
class TransportListener;
class TransportImpl;
class AudioDeviceImpl;
class LocalVideoDeviceImpl;
enum class EncryptionType;

class Transport : private RtcObject<TransportImpl, CreateSdpListener, SetSdpListener>
{
public:
    Transport(SignalTarget target, TransportListener* listener,
              const webrtc::scoped_refptr<PeerConnectionFactory>& pcf,
              const webrtc::PeerConnectionInterface::RTCConfiguration& conf,
              const std::string& identity,
              const std::string& prefferedAudioCodec = {},
              const std::string& prefferedVideoCodec = {},
              const std::shared_ptr<Bricks::Logger>& logger = {});
    ~Transport() override;
    SignalTarget target() const noexcept { return _target; }
    // config & media (fully async)
    bool setConfiguration(const webrtc::PeerConnectionInterface::RTCConfiguration& config);
    bool createDataChannel(const std::string& label,
                           const webrtc::DataChannelInit& init = {});
    void addTrack(std::shared_ptr<AudioDeviceImpl> device,
                  EncryptionType encryption,
                  const webrtc::RtpTransceiverInit& init = {});
    void addTrack(std::shared_ptr<LocalVideoDeviceImpl> device,
                  EncryptionType encryption,
                  const webrtc::RtpTransceiverInit& init = {});
    bool removeTrack(const std::string& id);
    void addIceCandidate(std::unique_ptr<webrtc::IceCandidateInterface> candidate);
    // stats
    void queryReceiverStats(const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback,
                            const webrtc::scoped_refptr<webrtc::RtpReceiverInterface>& receiver = {}) const;
    void querySenderStats(const webrtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback,
                          const webrtc::scoped_refptr<webrtc::RtpSenderInterface>& sender = {}) const;
    // SDP manipulations
    void createOffer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void createAnswer(const webrtc::PeerConnectionInterface::RTCOfferAnswerOptions& options = {});
    void setLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    void setRemoteDescription(std::unique_ptr<webrtc::SessionDescriptionInterface> desc);
    const webrtc::SessionDescriptionInterface* localDescription() const;
    const webrtc::SessionDescriptionInterface* remoteDescription() const;
    // state
    webrtc::PeerConnectionInterface::PeerConnectionState state() const noexcept;
    webrtc::PeerConnectionInterface::IceConnectionState iceConnectionState() const noexcept;
    webrtc::PeerConnectionInterface::SignalingState signalingState() const noexcept;
    webrtc::PeerConnectionInterface::IceGatheringState iceGatheringState() const noexcept;
    bool iceConnected() const noexcept;
    bool closed() const noexcept;
    // sync getters
    std::vector<webrtc::scoped_refptr<webrtc::RtpTransceiverInterface>> transceivers() const;
    std::vector<webrtc::scoped_refptr<webrtc::RtpReceiverInterface>> receivers() const;
    std::vector<webrtc::scoped_refptr<webrtc::RtpSenderInterface>> senders() const;
    // audio
    // Enable/disable playout of received audio streams. Enabled by default. Note
    // that even if playout is enabled, streams will only be played out if the
    // appropriate SDP is also applied. Setting `playout` to false will stop
    // playout of the underlying audio device but starts a task which will poll
    // for audio data every 10ms to ensure that audio processing happens and the
    // audio statistics are updated.
    void setAudioPlayout(bool playout);
    // Enable/disable recording of transmitted audio streams. Enabled by default.
    // Note that even if recording is enabled, streams will only be recorded if
    // the appropriate SDP is also applied.
    void setAudioRecording(bool recording);
    // getStats()
    bool valid() const;
    explicit operator bool() const { return valid(); }
    // Terminates all media, closes the transports, and in general releases any
    // resources used by the Transport. This is an irreversible operation.
    //
    // Note that after this method completes, the Transport will no longer
    // use the TransportListener interface passed in on construction, and
    // thus the listener object can be safely destroyed.
    void close();
private:
    std::unique_ptr<webrtc::SessionDescriptionInterface> patch(std::unique_ptr<webrtc::SessionDescriptionInterface> desc) const;
    template <class TMediaDevice>
    void addTransceiver(std::shared_ptr<TMediaDevice> device, EncryptionType encryption,
                        const webrtc::RtpTransceiverInit& init);
    // impl. of CreateSdpObserver
    void onSuccess(std::unique_ptr<webrtc::SessionDescriptionInterface> desc) final;
    void onFailure(webrtc::SdpType type, webrtc::RTCError error) final;
    // impl. of SetSdpListener
    void onCompleted(bool local) final;
    void onFailure(bool local, webrtc::RTCError error) final;
private:
    const SignalTarget _target;
    const std::string _prefferedAudioCodec;
    const std::string _prefferedVideoCodec;
    webrtc::scoped_refptr<CreateSdpObserver> _offerCreationObserver;
    webrtc::scoped_refptr<CreateSdpObserver> _answerCreationObserver;
    webrtc::scoped_refptr<SetLocalSdpObserver> _setLocalSdpObserver;
    webrtc::scoped_refptr<SetRemoteSdpObserver> _setRemoteSdpObserver;
};

} // namespace LiveKitCpp
