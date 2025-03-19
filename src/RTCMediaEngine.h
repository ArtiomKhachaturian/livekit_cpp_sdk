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
#pragma once // RTCMediaEngine.h
#ifdef WEBRTC_AVAILABLE
#include "Loggable.h"
#include "LocalTrackManager.h"
#include "SafeScopedRefPtr.h"
#include "SignalServerListener.h"
#include "TransportManagerListener.h"
#include "RemoteParticipants.h"
#include "RemoteParticipantsListener.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace LiveKitCpp
{

class LocalTrack;
class LocalParticipant;
class LocalParticipantImpl;
struct AddTrackRequest;
struct MuteTrackRequest;

class RTCMediaEngine : protected Bricks::LoggableS<SignalServerListener>,
                       protected TransportManagerListener,
                       protected LocalTrackManager,
                       protected RemoteParticipantsListener
{
public:
    std::shared_ptr<LocalParticipant> localParticipant() const;
    const auto& remoteParticipants() const noexcept { return _remoteParicipants; }
protected:
    enum class SendResult
    {
        Ok,
        TransportError,
        TransportClosed
    };
protected:
    RTCMediaEngine(const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCMediaEngine() override;
    void addLocalResourcesToTransport();
    void cleanupLocalResources();
    void cleanupRemoteResources();
    std::vector<webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>> pendingLocalMedia();
    virtual SendResult sendAddTrack(const AddTrackRequest& request) const = 0;
    virtual SendResult sendMuteTrack(const MuteTrackRequest& request) const = 0;
    virtual bool closed() const = 0;
    // impl. of SignalServerListener
    void onJoin(const JoinResponse& response) override;
    void onTrackPublished(const TrackPublishedResponse& published) final;
    void onTrackUnpublished(const TrackUnpublishedResponse& unpublished) final;
    void onUpdate(const ParticipantUpdate& update) final;
    // impl. of TransportManagerListener
    void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender) override;
    void onStateChange(webrtc::PeerConnectionInterface::PeerConnectionState,
                       webrtc::PeerConnectionInterface::PeerConnectionState publisherState,
                       webrtc::PeerConnectionInterface::PeerConnectionState subscriberState) override;
    void onLocalTrackAddFailure(const std::string& id, cricket::MediaType,
                                const std::vector<std::string>&, webrtc::RTCError) override;
    void onLocalTrackRemoved(const std::string& id, cricket::MediaType) override;
    void onRemoteTrackAdded(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
    void onRemotedTrackRemoved(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
private:
    // search by cid or sid
    void sendAddTrack(const LocalTrack* track);
    // impl. LocalTrackManager
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
private:
    const std::shared_ptr<LocalParticipantImpl> _localParticipant;
    RemoteParticipants _remoteParicipants;
};

} // namespace LiveKitCpp
#endif
