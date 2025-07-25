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
#pragma once // LocalParticipant.h
#include "AesCgmCryptorObserver.h"
#include "Listener.h"
#include "Loggable.h"
#include "ParticipantAccessor.h"
#include "SafeObjAliases.h"
#include "livekit/rtc/Participant.h"
#include "livekit/rtc/ParticipantListener.h"
#include "livekit/signaling/sfu/ParticipantInfo.h"
#include <api/media_types.h>
#include <api/scoped_refptr.h>
#include <atomic>
#include <optional>
#include <unordered_map>

namespace webrtc {
class AudioTrackInterface;
class RtpSenderInterface;
}

namespace Bricks {
class Logger;
}

namespace LiveKitCpp
{

class AudioDevice;
class AudioDeviceImpl;
class LocalVideoDeviceImpl;
class LocalVideoDevice;
class ParticipantListener;
class PeerConnectionFactory;
class TransportManager;
enum class EncryptionType;

class LocalParticipant : public Bricks::LoggableS<Participant, AesCgmCryptorObserver, ParticipantAccessor>
{
    using Base = Bricks::LoggableS<Participant, AesCgmCryptorObserver, ParticipantAccessor>;
    template <class T> using DeviceData = std::pair<std::shared_ptr<T>, EncryptionType>;
    template <class T> using Devices = std::unordered_map<std::string, DeviceData<T>>;
public:
    LocalParticipant(PeerConnectionFactory* pcf,
                     const Participant* session,
                     const std::shared_ptr<Bricks::Logger>& logger = {});
    ~LocalParticipant() final { reset(); }
    void reset();
    std::shared_ptr<AudioDeviceImpl> addDevice(std::unique_ptr<AudioDevice> device,
                                               EncryptionType encryption);
    std::shared_ptr<LocalVideoDeviceImpl> addDevice(std::unique_ptr<LocalVideoDevice> device,
                                                    EncryptionType encryption);
    bool removeDevice(const std::string& id);
    size_t devicesCount() const;
    void addDevicesToTransportManager(TransportManager* manager) const;
    std::optional<bool> stereoRecording() const;
    void setListener(ParticipantListener* listener) { _listener = listener; }
    void setInfo(const ParticipantInfo& info);
    // impl. of Participant
    std::string sid() const final { return _sid(); }
    std::string identity() const final { return _identity(); }
    std::string name() const final { return _name(); }
    std::string metadata() const final { return _metadata(); }
    ParticipantKind kind() const final { return _kind; }
    // impl. of ParticipantImpl
    void setSpeakerChanges(float level, bool active) const final;
    void setConnectionQuality(ConnectionQuality quality, float score) final;
private:
    bool removeAudioDevice(const std::string& id);
    bool removeVideoDevice(const std::string& id);
    void addAudioDevicesToTransportManager(TransportManager* manager) const;
    void addVideoDevicesToTransportManager(TransportManager* manager) const;
    template <class Method, typename... Args>
    void notify(const Method& method, Args&&... args) const;
    // impl. of AesCgmCryptorObserver
    void onEncryptionStateChanged(webrtc::MediaType mediaType, const std::string&,
                                  const std::string& trackId, AesCgmCryptorState state) final;
private:
    const webrtc::scoped_refptr<PeerConnectionFactory> _pcf;
    Bricks::SafeObj<const Participant*> _session;
    Bricks::Listener<ParticipantListener*> _listener;
    Bricks::SafeObj<Devices<AudioDeviceImpl>> _audioDevices;
    Bricks::SafeObj<Devices<LocalVideoDeviceImpl>> _videoDevices;
    Bricks::SafeObj<std::string> _sid;
    Bricks::SafeObj<std::string> _identity;
    Bricks::SafeObj<std::string> _name;
    Bricks::SafeObj<std::string> _metadata;
    std::atomic<ParticipantKind> _kind = ParticipantKind::Standard;
};

} // namespace LiveKitCpp
