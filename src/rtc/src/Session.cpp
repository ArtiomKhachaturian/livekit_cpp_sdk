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
#include "CameraTrackImpl.h"
#include "RTCEngine.h"
#include "StatsSourceImpl.h"
#include "PeerConnectionFactory.h"
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{

struct Session::Impl
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
    return _impl->_engine.localAudioTracksCount();
}

size_t Session::localVideoTracksCount() const
{
    return _impl->_engine.localVideoTracksCount();
}

std::shared_ptr<AudioTrack> Session::addAudioTrack(std::shared_ptr<AudioDevice> device)
{
    return _impl->_engine.addLocalAudioTrack(std::move(device));
}

std::shared_ptr<CameraTrack> Session::addCameraTrack(std::shared_ptr<CameraDevice> device)
{
    return _impl->_engine.addLocalCameraTrack(std::move(device));
}

void Session::removeAudioTrack(std::shared_ptr<AudioTrack> track)
{
    _impl->_engine.removeLocalAudioTrack(std::move(track));
}

void Session::removeVideoTrack(std::shared_ptr<VideoTrack> track)
{
    _impl->_engine.removeLocalVideoTrack(std::move(track));
}

std::shared_ptr<AudioTrack> Session::audioTrack(size_t index) const
{
    return _impl->_engine.localAudioTrack(index);
}

std::shared_ptr<VideoTrack> Session::videoTrack(size_t index) const
{
    return _impl->_engine.localVideoTrack(index);
}

size_t Session::remoteParticipantsCount() const
{
    return _impl->_engine.remoteParticipants().count();
}

std::shared_ptr<RemoteParticipant> Session::remoteParticipant(size_t index) const
{
    return _impl->_engine.remoteParticipants().at(index);
}

std::shared_ptr<RemoteParticipant> Session::remoteParticipant(const std::string& sid) const
{
    return _impl->_engine.remoteParticipants().at(sid);
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

void Session::enableAesCgmForLocalMedia(bool enable)
{
    _impl->_engine.enableAesCgmForLocalMedia(enable);
}

bool Session::aesCgmEnabledForLocalMedia() const
{
    return _impl->_engine.aesCgmEnabledForLocalMedia();
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
    return _impl->_engine.localParticipant().sid();
}

std::string Session::identity() const
{
    return _impl->_engine.localParticipant().identity();
}

std::string Session::name() const
{
    return _impl->_engine.localParticipant().name();
}

std::string Session::metadata() const
{
    return _impl->_engine.localParticipant().metadata();
}

ParticipantKind Session::kind() const
{
    return _impl->_engine.localParticipant().kind();
}

void Session::addListener(StatsListener* listener)
{
    _impl->_statsCollector->addListener(listener);
}

void Session::removeListener(StatsListener* listener)
{
    _impl->_statsCollector->removeListener(listener);
}

void Session::queryStats() const
{
    _impl->queryStats();
}

std::unique_ptr<KeyProvider> Session::createProvider(KeyProviderOptions options) const
{
    return std::make_unique<DefaultKeyProvider>(std::move(options), _impl->_engine.logger());
}

Session::Impl::Impl(Options options,
                    PeerConnectionFactory* pcf,
                    const Participant* session,
                    std::unique_ptr<Websocket::EndPoint> socket,
                    const std::shared_ptr<Bricks::Logger>& logger)
    : _statsCollector(webrtc::make_ref_counted<StatsSourceImpl>())
    , _engine(std::move(options), pcf, session, std::move(socket), logger)
{
}

} // namespace LiveKitCpp
