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
#pragma once // RTCEngine.h
#include "RtcObject.h"
#include "livekit/rtc/Options.h"
#include <api/scoped_refptr.h>
#include <atomic>
#include <string>
#include <vector>

namespace Bricks {
class Logger;
}

namespace Websocket {
class EndPoint;
}

namespace webrtc {
class MediaStreamTrackInterface;
class RTCStatsCollectorCallback;
class DataChannelInterface;
class RtpSenderInterface;
}

namespace LiveKitCpp
{

class AudioTrack;
class AudioDevice;
class LocalVideoDevice;
class CameraTrackImpl;
class LocalAudioTrackImpl;
class LocalVideoTrackImpl;
class LocalParticipant;
class KeyProvider;
class PeerConnectionFactory;
class TransportManager;
class Participant;
class RemoteParticipants;
class RemoteParticipant;
class RTCEngineImpl;
class SessionListener;
class LocalAudioTrack;
class LocalVideoTrack;
enum class SessionState;
enum class EncryptionType;

// https://github.com/livekit/client-sdk-js/blob/main/src/room/RTCEngine.ts
class RTCEngine : public RtcObject<RTCEngineImpl>
{
public:
    RTCEngine(Options options,
              PeerConnectionFactory* pcf,
              const Participant* session,
              std::unique_ptr<Websocket::EndPoint> socket,
              const std::shared_ptr<Bricks::Logger>& logger = {});
    ~RTCEngine();
    void setAudioPlayout(bool playout);
    bool audioPlayoutEnabled() const;
    void setAudioRecording(bool recording);
    bool audioRecordingEnabled() const;
    SessionState state() const noexcept;
    bool connect(std::string url, std::string authToken);
    void disconnect();
    void setListener(SessionListener* listener);
    void setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider);
    bool sendUserPacket(std::string payload, bool reliable,
                        const std::string& topic = {},
                        const std::vector<std::string>& destinationSids = {},
                        const std::vector<std::string>& destinationIdentities = {}) const;
    bool sendChatMessage(std::string message,
                         bool deleted,
                         bool generated,
                         const std::vector<std::string>& destinationIdentities = {}) const;
    void queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const;
    std::shared_ptr<LocalAudioTrackImpl> addLocalAudioTrack(std::shared_ptr<AudioDevice> device,
                                                            EncryptionType encryption);
    std::shared_ptr<LocalVideoTrackImpl> addLocalVideoTrack(std::shared_ptr<LocalVideoDevice> device,
                                                            EncryptionType encryption);
    bool removeLocalAudioTrack(std::shared_ptr<LocalAudioTrack> track);
    bool removeLocalVideoTrack(std::shared_ptr<LocalVideoTrack> track);
    std::shared_ptr<const LocalParticipant> localParticipant() const;
    std::shared_ptr<const RemoteParticipants> remoteParticipants() const;
    void setPrefferedVideoEncoder(const std::string& encoder);
    void setPrefferedAudioEncoder(const std::string& encoder);
    std::string prefferedVideoEncoder() const;
    std::string prefferedAudioEncoder() const;
};

} // namespace LiveKitCpp
