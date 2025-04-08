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
#pragma once // Session.h
#include "LiveKitClientExport.h"
#include "Options.h"
#include "SessionState.h"
#include "RemoteParticipant.h"
#include "e2e/KeyProvider.h"
#include "e2e/KeyProviderOptions.h"
#include "media/AudioTrack.h"
#include "media/CameraTrack.h"
#include "stats/StatsSource.h"
#include <memory>
#include <string>
#include <vector>

namespace Websocket {
class EndPoint;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AudioDevice;
class CameraDevice;
class RemoteParticipant;
class SessionListener;
class PeerConnectionFactory;

class LIVEKIT_CLIENT_API Session : public Participant, public StatsSource
{
    friend class Service;
    struct Impl;
public:
    ~Session();
    // local media
    // Enable/disable playout of received audio streams. Enabled by default. Note
    // that even if playout is enabled, streams will only be played out if the
    // appropriate SDP is also applied. Setting `playout` to false will stop
    // playout of the underlying audio device but starts a task which will poll
    // for audio data every 10ms to ensure that audio processing happens and the
    // audio statistics are updated.
    void setAudioPlayout(bool playout);
    bool audioPlayoutEnabled() const;
    // Enable/disable recording of transmitted audio streams. Enabled by default.
    // Note that even if recording is enabled, streams will only be recorded if
    // the appropriate SDP is also applied.
    void setAudioRecording(bool recording);
    bool audioRecordingEnabled() const;
    size_t localAudioTracksCount() const;
    size_t localVideoTracksCount() const;
    std::shared_ptr<AudioTrack> addAudioTrack(std::shared_ptr<AudioDevice> device);
    std::shared_ptr<CameraTrack> addCameraTrack(std::shared_ptr<CameraDevice> device);
    void removeAudioTrack(const std::shared_ptr<AudioTrack>& track);
    void removeVideoTrack(const std::shared_ptr<VideoTrack>& track);
    std::shared_ptr<AudioTrack> audioTrack(size_t index) const;
    std::shared_ptr<VideoTrack> videoTrack(size_t index) const;
    // remote
    size_t remoteParticipantsCount() const;
    // given participant by index or server ID
    std::shared_ptr<RemoteParticipant> remoteParticipant(size_t index) const;
    std::shared_ptr<RemoteParticipant> remoteParticipant(const std::string& sid) const;
    // e2e
    void setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider = {});
    void setAesCgmKeyProvider(KeyProviderOptions options);
    void setAesCgmKeyProvider(KeyProviderOptions options,
                              std::string_view sharedKey,
                              const std::optional<uint8_t>& sharedKeyIndex = {});
    void setAesCgmKeyProvider(KeyProviderOptions options,
                              std::vector<uint8_t> sharedKey,
                              const std::optional<uint8_t>& sharedKeyIndex = {});
    void enableAesCgmForLocalMedia(bool enable);
    bool aesCgmEnabledForLocalMedia() const; // true by default
    // chat & data channels
    /**
      * Publish a new data payload to the room. Data will be forwarded to each
      * participant in the room if the destination field in publishOptions is empty
      *
      * @param data Uint8Array of the payload. To send string data, use TextEncoder.encode
      * @param options optionally specify a `reliable`, `topic` and `destination`
      */
    /**
      * whether to send this as reliable or lossy.
      * For data that you need delivery guarantee (such as chat messages), use Reliable.
      * For data that should arrive as quickly as possible, but you are ok with dropped
      * packets, use Lossy.
      */
    /**
      * the identities of participants who will receive the message, will be sent to every one if empty
      */
    /** the topic under which the message gets published */
    bool sendUserPacket(std::string payload,
                        bool reliable = false,
                        const std::vector<std::string>& destinationIdentities = {},
                        const std::string& topic = {});
    // [deleted] true to remove message
    bool sendChatMessage(std::string message, bool deleted = false);
    // connect & state
    SessionState state() const;
    bool connect(std::string host, std::string authToken);
    void disconnect();
    void setListener(SessionListener* listener = nullptr);
    // impl. of Participant
    std::string sid() const final;
    std::string identity() const final;
    std::string name() const final;
    std::string metadata() const final;
    ParticipantKind kind() const final;
    // impl. of StatsSource
    void addListener(StatsListener* listener) final;
    void removeListener(StatsListener* listener) final;
    void queryStats() const final;
private:
    Session(std::unique_ptr<Websocket::EndPoint> socket,
            PeerConnectionFactory* pcf, Options options,
            const std::shared_ptr<Bricks::Logger>& logger = {});
    std::unique_ptr<KeyProvider> createProvider(KeyProviderOptions options) const;
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
