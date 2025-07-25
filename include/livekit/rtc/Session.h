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
#include "livekit/rtc/LiveKitRtcExport.h"
#include "livekit/rtc/Options.h"
#include "livekit/rtc/SessionState.h"
#include "livekit/rtc/RemoteParticipant.h"
#include "livekit/rtc/e2e/KeyProvider.h"
#include "livekit/rtc/e2e/KeyProviderOptions.h"
#include "livekit/rtc/media/LocalAudioTrack.h"
#include "livekit/rtc/media/LocalVideoTrack.h"
#include "livekit/rtc/stats/StatsSource.h"
#include "livekit/signaling/sfu/EncryptionType.h"
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
class LocalVideoDevice;
class RemoteParticipant;
class SessionListener;
class PeerConnectionFactory;

class LIVEKIT_RTC_API Session : public Participant, public StatsSource
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
    std::string addTrackDevice(std::unique_ptr<AudioDevice> device,
                               EncryptionType encryption = EncryptionType::None);
    std::string addTrackDevice(std::unique_ptr<LocalVideoDevice> device,
                               EncryptionType encryption = EncryptionType::None);
    void removeTrackDevice(const std::string& deviceId);
    size_t trackDevicesCount();
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
    bool sendUserPacket(std::string payload, bool reliable,
                        const std::string& topic = {},
                        const std::vector<std::string>& destinationSids = {},
                        const std::vector<std::string>& destinationIdentities = {});
    // [deleted] true to remove message
    bool sendChatMessage(std::string message,
                         bool deleted = false,
                         bool generated = false,
                         const std::vector<std::string>& destinationIdentities = {});
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
    void addStatsListener(StatsListener* listener) final;
    void removeStatsListener(StatsListener* listener) final;
    void queryStats() const final;
private:
    Session(std::unique_ptr<Websocket::EndPoint> socket,
            PeerConnectionFactory* pcf, Options options,
            bool disableAudioRed, const std::shared_ptr<Bricks::Logger>& logger = {});
    std::unique_ptr<KeyProvider> createProvider(KeyProviderOptions options) const;
private:
    const std::unique_ptr<Impl> _impl;
};

} // namespace LiveKitCpp
