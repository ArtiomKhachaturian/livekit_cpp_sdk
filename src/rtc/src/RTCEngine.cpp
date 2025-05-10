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
#include "RTCEngine.h"
#include "RTCEngineImpl.h"
#include "RemoteParticipantImpl.h"
#include "WebsocketEndPoint.h"
#include "Seq.h"
#include "TrackInfoSeq.h"
#include "livekit/rtc/e2e/KeyProvider.h"
#include "livekit/rtc/media/AudioDevice.h"
#include "livekit/rtc/media/LocalVideoDevice.h"

namespace
{

inline bool compareTrackInfo(const LiveKitCpp::TrackInfo& l, const LiveKitCpp::TrackInfo& r) {
    return l._sid == r._sid;
}

}

namespace LiveKitCpp
{

RTCEngine::RTCEngine(Options options,
                     PeerConnectionFactory* pcf,
                     const Participant* session,
                     std::unique_ptr<Websocket::EndPoint> socket,
                     const std::shared_ptr<Bricks::Logger>& logger)
    : RtcObject<RTCEngineImpl>(std::move(options), pcf, session, std::move(socket), logger)
{
}

RTCEngine::~RTCEngine()
{
    if (auto impl = dispose()) {
        impl->cleanup();
    }
}

void RTCEngine::setAudioPlayout(bool playout)
{
    if (const auto impl = loadImpl()) {
        impl->setAudioPlayout(playout);
    }
}

bool RTCEngine::audioPlayoutEnabled() const
{
    const auto impl = loadImpl();
    return impl && impl->audioPlayoutEnabled();
}

void RTCEngine::setAudioRecording(bool recording)
{
    if (const auto impl = loadImpl()) {
        impl->setAudioRecording(recording);
    }
}

bool RTCEngine::audioRecordingEnabled() const
{
    const auto impl = loadImpl();
    return impl && impl->audioRecordingEnabled();
}

SessionState RTCEngine::state() const noexcept
{
    if (const auto impl = loadImpl()) {
        return impl->state();
    }
    return SessionState::RtcClosed;
}

bool RTCEngine::connect(std::string url, std::string authToken)
{
    if (const auto impl = loadImpl()) {
        return impl->connect(std::move(url), std::move(authToken));
    }
    return false;
}

void RTCEngine::disconnect()
{
    if (const auto impl = loadImpl()) {
        impl->disconnect();
    }
}

void RTCEngine::setListener(SessionListener* listener)
{
    if (const auto impl = loadImpl()) {
        impl->setListener(listener);
    }
}

void RTCEngine::setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider)
{
    if (const auto impl = loadImpl()) {
        impl->setAesCgmKeyProvider(std::move(provider));
    }
}

bool RTCEngine::sendUserPacket(std::string payload, bool reliable, const std::string& topic,
                               const std::vector<std::string>& destinationSids,
                               const std::vector<std::string>& destinationIdentities) const
{
    if (const auto impl = loadImpl()) {
        return impl->sendUserPacket(std::move(payload), reliable, topic, destinationSids, destinationIdentities);
    }
    return false;
}

bool RTCEngine::sendChatMessage(std::string message, bool deleted, bool generated,
                                const std::vector<std::string>& destinationIdentities) const
{
    if (const auto impl = loadImpl()) {
        return impl->sendChatMessage(std::move(message), deleted, generated, destinationIdentities);
    }
    return false;
}

void RTCEngine::queryStats(const rtc::scoped_refptr<webrtc::RTCStatsCollectorCallback>& callback) const
{
    if (callback) {
        if (const auto impl = loadImpl()) {
            impl->queryStats(callback);
        }
    }
}

std::string RTCEngine::addTrackDevice(std::unique_ptr<AudioDevice> device, EncryptionType encryption)
{
    if (device) {
        if (const auto impl = loadImpl()) {
            return impl->addTrackDevice(std::move(device), encryption);
        }
    }
    return {};
}

std::string RTCEngine::addTrackDevice(std::unique_ptr<LocalVideoDevice> device, EncryptionType encryption)
{
    if (device) {
        if (const auto impl = loadImpl()) {
            return impl->addTrackDevice(std::move(device), encryption);
        }
    }
    return {};
}

void RTCEngine::removeTrackDevice(const std::string& deviceId)
{
    if (!deviceId.empty()) {
        if (const auto impl = loadImpl()) {
            impl->removeTrackDevice(deviceId);
        }
    }
}

std::shared_ptr<const LocalParticipant> RTCEngine::localParticipant() const
{
    if (const auto impl = loadImpl()) {
        return impl->localParticipant();
    }
    return {};
}

std::shared_ptr<const RemoteParticipants> RTCEngine::remoteParticipants() const
{
    if (const auto impl = loadImpl()) {
        return impl->remoteParticipants();
    }
    return {};
}

void findDifference(const std::vector<TrackInfo>& currentTracksInfo,
                    const std::vector<TrackInfo>& newTracksInfo,
                    std::vector<TrackInfo>* added,
                    std::vector<TrackInfo>* removed,
                    std::vector<TrackInfo>* updated)
{
    using SeqType = Seq<TrackInfo>;
    if (added) {
        *added = SeqType::difference<std::vector>(newTracksInfo, currentTracksInfo, compareTrackInfo);
    }
    if (removed) {
        *removed = SeqType::difference<std::vector>(currentTracksInfo, newTracksInfo, compareTrackInfo);
    }
    if (updated) {
        *updated = SeqType::intersection<std::vector>(currentTracksInfo, newTracksInfo, compareTrackInfo);
    }
}

} // namespace LiveKitCpp
