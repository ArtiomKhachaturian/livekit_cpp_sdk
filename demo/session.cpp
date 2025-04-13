#include "session.h"
#include "demoapp.h"
#include <livekit/rtc/Service.h>
#include <QThread>

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

}

Session::Session(QObject *parent)
    : QObject{parent}
    , _impl(create())
    , _localParticipant(new LocalParticipant(this))
{
    if (_impl) {
        QObject::connect(_localParticipant, &LocalParticipant::activeCameraChanged,
                         this, &Session::activeCameraChanged);
        QObject::connect(_localParticipant, &LocalParticipant::activeMicrophoneChanged,
                         this, &Session::activeMicrophoneChanged);
        QObject::connect(_localParticipant, &LocalParticipant::cameraDeviceInfoChanged,
                         this, &Session::cameraDeviceInfoChanged);
        QObject::connect(_localParticipant, &LocalParticipant::cameraOptionsChanged,
                         this, &Session::cameraOptionsChanged);
        QObject::connect(_localParticipant, &LocalParticipant::cameraMutedChanged,
                         this, &Session::cameraMutedChanged);
        QObject::connect(_localParticipant, &LocalParticipant::microphoneMutedChanged,
                         this, &Session::microphoneMutedChanged);
        QObject::connect(_localParticipant, &LocalParticipant::identityChanged,
                         this, &Session::identityChanged);
        _impl->setListener(this);
    }
}

Session::~Session()
{
    disconnectFromSfu();
    setActiveCamera(false);
    setActiveMicrophone(false);
    if (_impl) {
        _impl->setListener(nullptr);
        _localParticipant->disconnect(this);
    }
}

bool Session::connectToSfu(const QString& url, const QString& token)
{
    return _impl && _impl->connect(url.toStdString(), token.toStdString());
}

bool Session::activeCamera() const
{
    return _localParticipant->activeCamera();
}

bool Session::activeMicrophone() const
{
    return _localParticipant->activeMicrophone();
}

QString Session::cameraTrackId() const
{
    return _localParticipant->cameraTrackId();
}

QString Session::microphoneTrackId() const
{
    return _localParticipant->microphoneTrackId();
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

MediaDeviceInfo Session::cameraDeviceInfo() const
{
    return _localParticipant->cameraDeviceInfo();
}

CameraOptions Session::cameraOptions() const
{
    return _localParticipant->cameraOptions();
}

void Session::setActiveCamera(bool active)
{
    if (active != activeCamera()) {
        if (active) {
            if (const auto service = getService()) {
                auto device = service->createCamera(cameraDeviceInfo(), cameraOptions());
                const auto track = _impl->addCameraTrack(std::move(device));
                _localParticipant->activateCamera(track);
            }
        }
        else {
            _impl->removeVideoTrack(_localParticipant->deactivateCamera());
        }
    }
}

void Session::setActiveMicrophone(bool active)
{
    if (active != activeMicrophone()) {
        if (active) {
            if (const auto service = getService()) {
                auto device = service->createMicrophone();
                const auto track = _impl->addAudioTrack(std::move(device));
                _localParticipant->activateMicrophone(track);
            }
        }
        else {
            _impl->removeAudioTrack(_localParticipant->deactivateMicrophone());
        }
    }
}

void Session::setCameraDeviceInfo(const MediaDeviceInfo& info)
{
    _localParticipant->setCameraDeviceInfo(info);
}

void Session::setCameraOptions(const CameraOptions& options)
{
    _localParticipant->setCameraOptions(options);
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

std::unique_ptr<LiveKitCpp::Session> Session::create()
{
    if (const auto service = getService()) {
        return service->createSession();
    }
    return {};
}

void Session::onError(LiveKitCpp::LiveKitError liveKitError, const std::string& what)
{
    emit error(QString::fromStdString(LiveKitCpp::toString(liveKitError)), QString::fromStdString(what));
}

void Session::onChanged(const LiveKitCpp::Participant* participant)
{
    if (participant && participant == _impl.get()) {
        _localParticipant->setSid(QString::fromStdString(_impl->sid()));
        _localParticipant->setIdentity(QString::fromStdString(_impl->identity()));
        _localParticipant->setName(QString::fromStdString(_impl->name()));
    }
}

void Session::onStateChanged(LiveKitCpp::SessionState)
{
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
