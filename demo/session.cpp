#include "session.h"
#include "demoapp.h"
#include <livekit/rtc/Service.h>
#include <QThread>
#include <optional>

namespace
{

inline DemoApp* appInstance() {
    return qobject_cast<DemoApp*>(QCoreApplication::instance());
}

inline std::shared_ptr<LiveKitCpp::Service> getService() {
    if (const auto app = appInstance()) {
        return app->service().lock();
    }
    return {};
}

inline std::optional<LiveKitCpp::IceTransportPolicy> toIceTransportPolicy(const QString& policy) {
    const auto utf8Policy = policy.toStdString();
    if (!utf8Policy.empty()) {
        for (const auto policy : {LiveKitCpp::IceTransportPolicy::None,
                                  LiveKitCpp::IceTransportPolicy::Relay,
                                  LiveKitCpp::IceTransportPolicy::NoHost,
                                  LiveKitCpp::IceTransportPolicy::All}) {
            if (utf8Policy == toString(policy)) {
                return policy;
            }
        }
    }
    return std::nullopt;
}


}

Session::Session(QObject *parent)
    : QObject{parent}
    , _localParticipant(new LocalParticipant(this))
{
    QObject::connect(_localParticipant, &LocalParticipant::cameraDeviceInfoChanged,
                     this, &Session::cameraDeviceInfoChanged);
    QObject::connect(_localParticipant, &LocalParticipant::cameraOptionsChanged,
                     this, &Session::cameraOptionsChanged);
    QObject::connect(_localParticipant, &LocalParticipant::cameraMutedChanged,
                     this, &Session::cameraMutedChanged);

    QObject::connect(_localParticipant, &LocalParticipant::microphoneOptionsChanged,
                     this, &Session::microphoneOptionsChanged);
    QObject::connect(_localParticipant, &LocalParticipant::microphoneMutedChanged,
                     this, &Session::microphoneMutedChanged);

    QObject::connect(_localParticipant, &LocalParticipant::sharingDeviceInfoChanged,
                     this, &Session::sharingDeviceInfoChanged);
    QObject::connect(_localParticipant, &LocalParticipant::sharingOptionsChanged,
                     this, &Session::sharingOptionsChanged);
    QObject::connect(_localParticipant, &LocalParticipant::sharingMutedChanged,
                     this, &Session::sharingMutedChanged);


    QObject::connect(_localParticipant, &LocalParticipant::identityChanged,
                     this, &Session::identityChanged);

}

Session::~Session()
{
    disconnectFromSfu();
    setActiveCamera(false);
    setActiveMicrophone(false);
    resetSessionImpl();
    _localParticipant->disconnect(this);
}

bool Session::connectToSfu(const QString& url, const QString& token,
                           bool autoSubscribe, bool adaptiveStream,
                           bool e2e, const QString& iceTransportPolicy,
                           const QString& e2ePassPhrase)
{
    LiveKitCpp::Options options;
    options._autoSubscribe = autoSubscribe;
    options._adaptiveStream = adaptiveStream;
    if (const auto policy = toIceTransportPolicy(iceTransportPolicy)) {
        options._iceTransportPolicy = policy.value();
    }
    options._prefferedAudioEncoder = _prefferedAudioEncoder.toStdString();
    options._prefferedVideoEncoder = _prefferedVideoEncoder.toStdString();
    auto impl = create(std::move(options));
    if (impl) {
        impl->setAesCgmKeyProvider(LiveKitCpp::KeyProviderOptions::defaultOptions(),
                                   e2ePassPhrase.toStdString());
        if (e2e) {
            _encryption = LiveKitCpp::EncryptionType::Gcm;
        }
        else {
            _encryption = LiveKitCpp::EncryptionType::None;
        }
    }
    setSessionImpl(std::move(impl));
    return _impl && _impl->connect(url.toStdString(), token.toStdString());
}

QString Session::cameraTrackId() const
{
    return _localParticipant->cameraTrackId();
}

QString Session::microphoneTrackId() const
{
    return _localParticipant->microphoneTrackId();
}

QString Session::sharingTrackId() const
{
    return _localParticipant->sharingTrackId();
}

bool Session::connecting() const
{
    switch (state()) {
        case State::TransportConnecting:
        case State::TransportConnected:
        case State::RtcConnecting:
            return true;
        default:
            break;
    }
    return false;
}

bool Session::connected() const
{
    return State::RtcConnected == state();
}

Session::State Session::state() const
{
    if (_impl) {
        switch (_impl->state()) {
            case LiveKitCpp::SessionState::TransportConnecting:
                return State::TransportConnecting;
            case LiveKitCpp::SessionState::TransportConnected:
                return State::TransportConnected;
            case LiveKitCpp::SessionState::TransportDisconnecting:
                return State::TransportDisconnecting;
            case LiveKitCpp::SessionState::TransportDisconnected:
                return State::TransportDisconnected;
            case LiveKitCpp::SessionState::RtcConnecting:
                return State::RtcConnecting;
            case LiveKitCpp::SessionState::RtcConnected:
                return State::RtcConnected;
            case LiveKitCpp::SessionState::RtcDisconnected:
                return State::RtcDisconnected;
            case LiveKitCpp::SessionState::RtcClosed:
                return State::RtcClosed;
            default:
                break;
        }
    }
    return State::TransportDisconnected;
}

void Session::setActiveCamera(bool active)
{
    if (active != _activeCamera) {
        _activeCamera = active;
        if (active) {
            addCameraTrack();
        }
        else {
            removeCameraTrack();
        }
        emit activeCameraChanged();
    }
}

void Session::setActiveMicrophone(bool active)
{
    if (active != activeMicrophone()) {
        _activeMicrophone = active;
        if (active) {
            addMicrophoneTrack();
        }
        else {
            removeMicrophoneTrack();
        }
        emit activeMicrophoneChanged();
    }
}

void Session::setActiveSharing(bool active)
{
    if (active != activeSharing()) {
        _activeSharing = active;
        if (active) {
            addSharingTrack();
        }
        else {
            removeSharingTrack();
        }
        emit activeSharingChanged();
    }
}

void Session::setPrefferedVideoEncoder(const QString& encoder)
{
    if (_prefferedVideoEncoder != encoder) {
        _prefferedVideoEncoder = encoder;
    }
}

void Session::setPrefferedAudioEncoder(const QString& encoder)
{
    if (_prefferedAudioEncoder != encoder) {
        _prefferedAudioEncoder = encoder;
    }
}

void Session::disconnectFromSfu()
{
    if (_impl) {
        _impl->disconnect();
    }
}

bool Session::sendChatMessage(const QString& message)
{
    return _impl && _impl->sendChatMessage(message.toStdString());
}

std::unique_ptr<LiveKitCpp::Session> Session::create(LiveKitCpp::Options options)
{
    if (const auto service = getService()) {
        return service->createSession(std::move(options));
    }
    return {};
}

void Session::addCameraTrack()
{
    if (_impl) {
        if (const auto service = getService()) {
            auto device = service->createCamera(cameraDeviceInfo(), cameraOptions());
            const auto id = _impl->addTrackDevice(std::move(device), _encryption);
            _localParticipant->activateCamera(QString::fromStdString(id));
        }
    }
}

void Session::addMicrophoneTrack()
{
    if (_impl) {
        if (const auto service = getService()) {
            auto device = service->createMicrophone(microphoneOptions());
            const auto id = _impl->addTrackDevice(std::move(device), _encryption);
            _localParticipant->activateMicrophone(QString::fromStdString(id));
        }
    }
}

void Session::addSharingTrack()
{
    if (_impl) {
        if (const auto service = getService()) {
            auto device = service->createSharing(false, sharingDeviceInfo(), sharingOptions());
            const auto id = _impl->addTrackDevice(std::move(device), _encryption);
            _localParticipant->activateSharing(QString::fromStdString(id));
        }
    }
}

void Session::removeCameraTrack()
{
    const auto id = _localParticipant->cameraTrackId();
    if (!id.isEmpty()) {
        _localParticipant->deactivateCamera();
        if (_impl) {
            _impl->removeTrackDevice(id.toStdString());
        }
    }
}

void Session::removeMicrophoneTrack()
{
    const auto id = _localParticipant->microphoneTrackId();
    if (!id.isEmpty()) {
        _localParticipant->deactivateMicrophone();
        if (_impl) {
            _impl->removeTrackDevice(id.toStdString());
        }
    }
}

void Session::removeSharingTrack()
{
    const auto id = _localParticipant->sharingTrackId();
    if (!id.isEmpty()) {
        _localParticipant->deactivateSharing();
        if (_impl) {
            _impl->removeTrackDevice(id.toStdString());
        }
    }
}

void Session::setSessionImpl(std::unique_ptr<LiveKitCpp::Session> impl)
{
    if (_impl.get() != impl.get()) {
        removeCameraTrack();
        removeSharingTrack();
        removeMicrophoneTrack();
        if (_impl) {
            _impl->setListener(nullptr);
        }
        _impl = std::move(impl);
        if (_impl) {
            _impl->setListener(this);
            if (_activeCamera) {
                addCameraTrack();
            }
            if (_activeMicrophone) {
                addMicrophoneTrack();
            }
            if (_activeSharing) {
                addSharingTrack();
            }
        }
        emit stateChanged();
    }
}

void Session::onError(LiveKitCpp::LiveKitError liveKitError, const std::string& what)
{
    emit error(QString::fromStdString(LiveKitCpp::toString(liveKitError)), QString::fromStdString(what));
}

void Session::onLocalAudioTrackAdded(const std::shared_ptr<LiveKitCpp::LocalAudioTrack>& track)
{
    if (track) {
        switch (track->source()) {
        case LiveKitCpp::TrackSource::Microphone:
            _localParticipant->setMicrophoneTrack(track);
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    }
}

void Session::onLocalVideoTrackAdded(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& track)
{
    if (track) {
        switch (track->source()) {
        case LiveKitCpp::TrackSource::Camera:
            _localParticipant->setCameraTrack(track);
            break;
        case LiveKitCpp::TrackSource::ScreenShare:
            _localParticipant->setSharingTrack(track);
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    }

}

void Session::onLocalAudioTrackAddFailure(std::string id, std::string_view details)
{
    emit localMediaTrackAddFailure(true, QString::fromStdString(id),
                                   QString::fromUtf8(details.data(), details.size()));
}

void Session::onLocalVideoTrackAddFailure(std::string id, std::string_view details)
{
    emit localMediaTrackAddFailure(false, QString::fromStdString(id),
                                   QString::fromUtf8(details.data(), details.size()));
}

void Session::onLocalAudioTrackRemoved(std::string id)
{
    if (_localParticipant->microphoneTrackId().toStdString() == id) {
        _localParticipant->deactivateMicrophone();
    }
}

void Session::onLocalVideoTrackRemoved(std::string id)
{
    if (_localParticipant->cameraTrackId().toStdString() == id) {
        _localParticipant->deactivateCamera();
    }
    else if (_localParticipant->sharingTrackId().toStdString() == id) {
        _localParticipant->deactivateSharing();
    }
}

void Session::onSidChanged(const LiveKitCpp::Participant* participant)
{
    if (participant && participant == _impl.get()) {
        _localParticipant->setSid(QString::fromStdString(_impl->sid()));
    }
}

void Session::onIdentityChanged(const LiveKitCpp::Participant* participant)
{
    if (participant && participant == _impl.get()) {
        _localParticipant->setIdentity(QString::fromStdString(_impl->identity()));
    }
}

void Session::onNameChanged(const LiveKitCpp::Participant* participant)
{
    if (participant && participant == _impl.get()) {
        _localParticipant->setName(QString::fromStdString(_impl->name()));
    }
}

void Session::onSpeakerInfoChanged(const LiveKitCpp::Participant* participant,
                                   float level, bool active)
{
    if (participant && participant == _impl.get()) {
        _localParticipant->notifyThatSpeakerInfoChanged(level, active);
    }
}

void Session::onStateChanged(LiveKitCpp::SessionState state)
{
    switch (state) {
        case LiveKitCpp::SessionState::TransportDisconnected:
        case LiveKitCpp::SessionState::RtcClosed:
            QMetaObject::invokeMethod(this, &Session::resetSessionImpl);
            break;
        default:
            break;
    }
    emit stateChanged();
}

void Session::onChatMessageReceived(const LiveKitCpp::ChatMessage& message,
                                    const std::string& participantIdentity,
                                    const std::vector<std::string>&)
{
    emit chatMessageReceived(QString::fromStdString(participantIdentity),
                             QString::fromStdString(message._message),
                             message._deleted);
}

void Session::addRemoteParticipant(const QString& sid)
{
    if (_impl && !sid.isEmpty()) {
        if (const auto sdkParticipant = _impl->remoteParticipant(sid.toStdString())) {
            removeRemoteParticipant(sid);
            auto participant = new RemoteParticipant(sdkParticipant, this);
            _remoteParticipants[sid] = participant;
            emit remoteParticipantAdded(participant);
        }
    }
}

void Session::removeRemoteParticipant(const QString& sid)
{
    if (!sid.isEmpty()) {
        const auto it = _remoteParticipants.find(sid);
        if (it != _remoteParticipants.end()) {
            emit remoteParticipantRemoved(it.value());
            delete it.value();
            _remoteParticipants.erase(it);
        }
    }
}

void Session::onRemoteParticipantAdded(const std::string& sid)
{
    if (_impl && !sid.empty()) {
        QMetaObject::invokeMethod(this, &Session::addRemoteParticipant,
                                  QString::fromStdString(sid));
    }
}

void Session::onRemoteParticipantRemoved(const std::string& sid)
{
    if (_impl && !sid.empty()) {
        QMetaObject::invokeMethod(this, &Session::removeRemoteParticipant,
                                  QString::fromStdString(sid));
    }
}
