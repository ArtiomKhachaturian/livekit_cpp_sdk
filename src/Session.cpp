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
#include "Session.h"
#ifdef WEBRTC_AVAILABLE
#include "DefaultKeyProvider.h"
#include "LocalAudioTrackImpl.h"
#include "CameraTrackImpl.h"
#include "RTCEngine.h"
#include "StatsSourceImpl.h"
#include "PeerConnectionFactory.h"
#endif
#include "WebsocketEndPoint.h"

namespace LiveKitCpp
{

#ifdef WEBRTC_AVAILABLE
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

std::shared_ptr<AudioTrack> Session::addMicrophoneTrack()
{
    return _impl->_engine.addLocalMicrophoneTrack();
}

std::shared_ptr<CameraTrack> Session::addCameraTrack(const MediaDeviceInfo& info,
                                                     const CameraOptions& options)
{
    return _impl->_engine.addLocalCameraTrack(info, options);
}

void Session::removeAudioTrack(const std::shared_ptr<AudioTrack>& track)
{
    _impl->_engine.removeLocalAudioTrack(track);
}

void Session::removeVideoTrack(const std::shared_ptr<VideoTrack>& track)
{
    _impl->_engine.removeLocalVideoTrack(track);
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
                             const std::vector<std::string>& destinationIdentities,
                             const std::string& topic)
{
    return _impl->_engine.sendUserPacket(std::move(payload), reliable,
                                         destinationIdentities, topic);
}

bool Session::sendChatMessage(std::string message, bool deleted)
{
    return _impl->_engine.sendChatMessage(std::move(message), deleted);
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

#else
struct Session::Impl {}; // stub

Session::Session(std::unique_ptr<Websocket::EndPoint>, PeerConnectionFactory*,
                 Options, const std::shared_ptr<Bricks::Logger>&) {}

Session::~Session() {}

void Session::setAudioPlayout(bool) {}

bool Session::audioPlayoutEnabled() const { return false; }

void Session::setAudioRecording(bool) {}

bool Session::audioRecordingEnabled() const { return false; }

size_t Session::localAudioTracksCount() const { return 0U; }

size_t Session::localVideoTracksCount() const { return 0U; }

std::shared_ptr<AudioTrack> Session::addMicrophoneTrack() { return {}; }

std::shared_ptr<CameraTrack> Session::addCameraTrack(const MediaDeviceInfo&,
                                                     const CameraOptions&) { return {}; }

void Session::removeAudioTrack(const std::shared_ptr<AudioTrack>&) {}

void Session::removeVideoTrack(const std::shared_ptr<VideoTrack>&) {}

std::shared_ptr<AudioTrack> Session::audioTrack(size_t) const { return {}; }

std::shared_ptr<VideoTrack> Session::videoTrack(size_t) const { return {}; }

size_t Session::remoteParticipantsCount() const { return 0U; }

std::shared_ptr<RemoteParticipant> Session::remoteParticipant(size_t) const
{
    return {};
}

std::shared_ptr<RemoteParticipant> Session::remoteParticipant(const std::string&) const
{
    return {};
}

void Session::setAesCgmKeyProvider(std::unique_ptr<KeyProvider>) {}

void Session::setAesCgmKeyProvider(KeyProviderOptions) {}

void Session::setAesCgmKeyProvider(KeyProviderOptions, std::string_view,
                                   const std::optional<uint8_t>&) {}

void Session::setAesCgmKeyProvider(KeyProviderOptions, std::vector<uint8_t>,
                                   const std::optional<uint8_t>&) {}

void Session::enableAesCgmForLocalMedia(bool) {}

bool Session::aesCgmEnabledForLocalMedia() const { return false; }

bool Session::sendUserPacket(std::string, bool,
                             const std::vector<std::string>&,
                             const std::string&) { return false; }

bool Session::sendChatMessage(std::string, bool) { return false; }

SessionState Session::state() const { return SessionState::TransportDisconnected; }

bool Session::connect(std::string, std::string) { return false; }

void Session::disconnect() {}

void Session::setListener(SessionListener*) {}

std::string Session::sid() const { return {}; }

std::string Session::identity() const { return {}; }

std::string Session::name() const { return {}; }

std::string Session::metadata() const { return {}; }

ParticipantKind Session::kind() const { return ParticipantKind::Standard; }

void Session::addListener(StatsListener*) {}

void Session::removeListener(StatsListener*) {}

void Session::queryStats() const {}

std::unique_ptr<KeyProvider> Session::createProvider(KeyProviderOptions) const { return {}; }
#endif
} // namespace LiveKitCpp
