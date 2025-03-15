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
#include "Loggable.h"
#include "TransportManagerListener.h"
#include "LocalAudioTrackImpl.h"
#include "CameraTrackImpl.h"
#include "LocalTrackManager.h"
#include "SafeScopedRefPtr.h"
#include "SignalServerListener.h"
#include "DataChannelListener.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace LiveKitCpp
{

class RemoteTrack;
struct AddTrackRequest;
struct MuteTrackRequest;

class RTCMediaEngine : protected Bricks::LoggableS<SignalServerListener>,
                       protected TransportManagerListener,
                       protected LocalTrackManager,
                       private DataChannelListener
{
    // key is channel label
    using DataChannels = std::unordered_map<std::string, rtc::scoped_refptr<DataChannel>>;
    // key is sid
    using RemoteTracks = std::unordered_map<std::string, std::unique_ptr<RemoteTrack>>;
    // key is cid (track id), for LocalTrackManager [publishMedia] / [unpublishMedia]
    using PendingLocalMedias = std::unordered_map<std::string, webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>>;
protected:
    enum class SendResult
    {
        Ok,
        TransportError,
        TransportClosed
    };
public:
    void unmuteMicrophone(bool unmute = true) { muteMicrophone(!unmute); }
    void muteMicrophone(bool mute = true) { _microphone.mute(mute); }
    void unmuteCamera(bool unmute = true) { muteCamera(!unmute); }
    void muteCamera(bool mute = true) { _camera.mute(mute); }
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
    // impl. LocalTrackManager
    bool addLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;
    bool removeLocalMedia(const webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface>& track) override;
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
    void onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel) override;
    void onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel) override;
private:
    // search by cid or sid
    LocalTrack* localTrack(const std::string& id, bool cid);
    LocalTrack* localTrack(const rtc::scoped_refptr<webrtc::RtpSenderInterface>& sender);
    void sendAddTrack(const LocalTrack* track);
    void addDataChannelToList(rtc::scoped_refptr<DataChannel> channel, DataChannels& list);
    // impl. of SignalServerListener
    void onTrackPublished(const TrackPublishedResponse& published) final;
    void onTrackUnpublished(const TrackUnpublishedResponse& unpublished) final;
    // impl. LocalTrackManager
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
    // impl. of DataChannelListener
    void onStateChange(DataChannel* channel) final;
    void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) final;
    void onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize) final;
    void onSendError(DataChannel* channel, webrtc::RTCError error) final;
private:
    Bricks::SafeObj<PendingLocalMedias> _pendingLocalMedias;
    Bricks::SafeObj<DataChannels> _localDCs;
    Bricks::SafeObj<DataChannels> _remoteDCs;
    Bricks::SafeObj<RemoteTracks> _remoteTracks;
    LocalAudioTrackImpl _microphone;
    CameraTrackImpl _camera;
};

} // namespace LiveKitCpp
