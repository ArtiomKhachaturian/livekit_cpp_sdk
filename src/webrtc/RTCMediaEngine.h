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
#include "LocalAudioTrack.h"
#include "LocalTrackManager.h"
#include "SafeScopedRefPtr.h"
#include "SignalServerListener.h"
#include "DataChannelListener.h"
#include <unordered_map>

namespace LiveKitCpp
{

class PeerConnectionFactory;
struct AddTrackRequest;
struct MuteTrackRequest;

class RTCMediaEngine : protected Bricks::LoggableS<SignalServerListener>,
                       protected TransportManagerListener,
                       protected LocalTrackManager,
                       private DataChannelListener
{
    using DataChannels = std::unordered_map<std::string, rtc::scoped_refptr<DataChannel>>;
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
protected:
    RTCMediaEngine(PeerConnectionFactory* pcf, const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCMediaEngine() override;
    virtual void cleanup(bool /*error*/);
    const auto& peerConnectionFactory() const noexcept { return _pcf; }
    virtual SendResult sendAddTrack(const AddTrackRequest& request) const = 0;
    virtual SendResult sendMuteTrack(const MuteTrackRequest& request) const = 0;
    virtual bool closed() const = 0;
private:
    LocalTrack* localTrack(const std::string& id, cricket::MediaType type = cricket::MEDIA_TYPE_UNSUPPORTED);
    void sendAddTrack(const LocalTrack* track);
    void addDataChannelToList(rtc::scoped_refptr<DataChannel> channel, DataChannels& list);
    // impl. of SignalServerListener
    void onTrackPublished(uint64_t, const TrackPublishedResponse& published) final;
    // impl. of TransportManagerListener
    void onLocalTrackAdded(rtc::scoped_refptr<webrtc::RtpSenderInterface> sender) final;
    void onLocalTrackAddFailure(const std::string& id, cricket::MediaType type,
                                const std::vector<std::string>&, webrtc::RTCError) final;
    void onLocalTrackRemoved(const std::string& id, cricket::MediaType type) final;
    void onLocalDataChannelCreated(rtc::scoped_refptr<DataChannel> channel) final;
    void onRemoteDataChannelOpened(rtc::scoped_refptr<DataChannel> channel) final;
    // impl. LocalTrackFactory
    webrtc::scoped_refptr<webrtc::AudioTrackInterface> createAudio(const std::string& label,
                                                                   const cricket::AudioOptions& options) final;
    void notifyAboutMuteChanges(const std::string& trackSid, bool muted) final;
    // impl. of DataChannelListener
    void onStateChange(DataChannel* channel) final;
    void onMessage(DataChannel* channel, const webrtc::DataBuffer& buffer) final;
    void onBufferedAmountChange(DataChannel* channel, uint64_t sentDataSize) final;
private:
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    Bricks::SafeObj<DataChannels> _localDCs;
    Bricks::SafeObj<DataChannels> _remoteDCs;
    LocalAudioTrack _microphone;
};

} // namespace LiveKitCpp
