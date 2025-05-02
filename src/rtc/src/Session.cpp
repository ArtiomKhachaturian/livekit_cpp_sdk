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
#include "livekit/rtc/Session.h"
#include "DefaultKeyProvider.h"
#include "LocalAudioTrackImpl.h"
#include "LocalVideoTrackImpl.h"
#include "LocalParticipant.h"
#include "RemoteParticipantImpl.h"
#include "RemoteParticipants.h"
#include "RTCEngine.h"
#include "StatsSourceImpl.h"
#include "PeerConnectionFactory.h"
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{

struct Session::Impl : public Bricks::LoggableS<>
{
    const webrtc::scoped_refptr<StatsSourceImpl> _statsCollector;
    RTCEngine _engine;
    Impl(Options options,
         PeerConnectionFactory* pcf,
         const Participant* session,
         std::unique_ptr<Websocket::EndPoint> socket,
         const std::shared_ptr<Bricks::Logger>& logger);
    ~Impl() { _statsCollector->clearListeners(); }
    void queryStats() const { _engine.queryStats(_statsCollector); }
};

Session::Session(std::unique_ptr<Websocket::EndPoint> socket,
                 PeerConnectionFactory* pcf, Options options,
                 const std::shared_ptr<Bricks::Logger>& logger)
    : _impl(std::make_unique<Impl>(std::move(options), pcf, this, std::move(socket), logger))
{
}

Session::~Session()
{
    disconnect();
}

void Session::setAudioPlayout(bool playout)
{
    _impl->_engine.setAudioPlayout(playout);
}

bool Session::audioPlayoutEnabled() const
{
    return _impl->_engine.audioPlayoutEnabled();
}

void Session::setAudioRecording(bool recording)
{
    _impl->_engine.setAudioRecording(recording);
}

bool Session::audioRecordingEnabled() const
{
    return _impl->_engine.audioRecordingEnabled();
}

size_t Session::localAudioTracksCount() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->audioTracksCount();
    }
    return 0U;
}

size_t Session::localVideoTracksCount() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->videoTracksCount();
    }
    return 0U;
}

std::shared_ptr<LocalAudioTrack> Session::addAudioTrack(std::shared_ptr<AudioDevice> device,
                                                        EncryptionType encryption)
{
    return _impl->_engine.addLocalAudioTrack(std::move(device), encryption);
}

std::shared_ptr<LocalVideoTrack> Session::addVideoTrack(std::shared_ptr<LocalVideoDevice> device,
                                                        EncryptionType encryption)
{
    return _impl->_engine.addLocalVideoTrack(std::move(device), encryption);
}

void Session::removeAudioTrack(std::shared_ptr<LocalAudioTrack> track)
{
    _impl->_engine.removeLocalAudioTrack(std::move(track));
}

void Session::removeVideoTrack(std::shared_ptr<LocalVideoTrack> track)
{
    _impl->_engine.removeLocalVideoTrack(std::move(track));
}

std::shared_ptr<LocalAudioTrack> Session::audioTrack(size_t index) const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->audioTrack(index);
    }
    return {};
}

std::shared_ptr<LocalVideoTrack> Session::videoTrack(size_t index) const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->videoTrack(index);
    }
    return {};
}

size_t Session::remoteParticipantsCount() const
{
    if (const auto participant = _impl->_engine.remoteParticipants()) {
        return participant->count();
    }
    return 0U;
}

std::shared_ptr<RemoteParticipant> Session::remoteParticipant(size_t index) const
{
    if (const auto participant = _impl->_engine.remoteParticipants()) {
        return participant->at(index);
    }
    return {};
}

std::shared_ptr<RemoteParticipant> Session::remoteParticipant(const std::string& sid) const
{
    if (const auto participant = _impl->_engine.remoteParticipants()) {
        return participant->at(sid);
    }
    return {};
}

void Session::setAesCgmKeyProvider(std::unique_ptr<KeyProvider> provider)
{
    _impl->_engine.setAesCgmKeyProvider(std::move(provider));
}

void Session::setAesCgmKeyProvider(KeyProviderOptions options)
{
    if (auto provider = createProvider(std::move(options))) {
        setAesCgmKeyProvider(std::move(provider));
    }
}

void Session::setAesCgmKeyProvider(KeyProviderOptions options,
                                   std::string_view sharedKey,
                                   const std::optional<uint8_t>& sharedKeyIndex)
{
    if (auto provider = createProvider(std::move(options))) {
        if (!provider->setSharedKey(std::move(sharedKey), sharedKeyIndex)) {
            // TODO: log warning
        }
        setAesCgmKeyProvider(std::move(provider));
    }
}

void Session::setAesCgmKeyProvider(KeyProviderOptions options,
                                   std::vector<uint8_t> sharedKey,
                                   const std::optional<uint8_t>& sharedKeyIndex)
{
    if (auto provider = createProvider(std::move(options))) {
        if (!provider->setSharedKey(std::move(sharedKey), sharedKeyIndex)) {
            // TODO: log warning
        }
        setAesCgmKeyProvider(std::move(provider));
    }
}

bool Session::sendUserPacket(std::string payload, bool reliable,
                             const std::string& topic,
                             const std::vector<std::string>& destinationSids,
                             const std::vector<std::string>& destinationIdentities)
{
    return _impl->_engine.sendUserPacket(std::move(payload), reliable,
                                         topic, destinationSids, destinationIdentities);
}

bool Session::sendChatMessage(std::string message, bool deleted, bool generated,
                              const std::vector<std::string>& destinationIdentities)
{
    return _impl->_engine.sendChatMessage(std::move(message),
                                          deleted, generated,
                                          destinationIdentities);
}

SessionState Session::state() const
{
    return _impl->_engine.state();
}

bool Session::connect(std::string host, std::string authToken)
{
    return _impl->_engine.connect(std::move(host), std::move(authToken));
}

void Session::disconnect()
{
    _impl->_engine.disconnect();
}

void Session::setListener(SessionListener* listener)
{
    _impl->_engine.setListener(listener);
}

std::string Session::sid() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->sid();
    }
    return {};
}

std::string Session::identity() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->identity();
    }
    return {};
}

std::string Session::name() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->name();
    }
    return {};
}

std::string Session::metadata() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->metadata();
    }
    return {};
}

ParticipantKind Session::kind() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->kind();
    }
    return ParticipantKind::Standard;
}

size_t Session::audioTracksCount() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->audioTracksCount();
    }
    return 0U;
}

size_t Session::videoTracksCount() const
{
    if (const auto participant = _impl->_engine.localParticipant()) {
        return participant->videoTracksCount();
    }
    return 0U;
}

void Session::addStatsListener(StatsListener* listener)
{
    _impl->_statsCollector->addListener(listener);
}

void Session::removeStatsListener(StatsListener* listener)
{
    _impl->_statsCollector->removeListener(listener);
}

void Session::queryStats() const
{
    _impl->queryStats();
}

std::unique_ptr<KeyProvider> Session::createProvider(KeyProviderOptions options) const
{
    return std::make_unique<DefaultKeyProvider>(std::move(options), _impl->logger());
}

Session::Impl::Impl(Options options,
                    PeerConnectionFactory* pcf,
                    const Participant* session,
                    std::unique_ptr<Websocket::EndPoint> socket,
                    const std::shared_ptr<Bricks::Logger>& logger)
    : Bricks::LoggableS<>(logger)
    , _statsCollector(webrtc::make_ref_counted<StatsSourceImpl>())
    , _engine(std::move(options), pcf, session, std::move(socket), logger)
{
}

} // namespace LiveKitCpp
