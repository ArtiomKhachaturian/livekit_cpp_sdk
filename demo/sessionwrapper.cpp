#include "sessionwrapper.h"

SessionWrapper::SessionWrapper(std::unique_ptr<LiveKitCpp::Session> impl, QObject *parent)
    : QObject{parent}
    , _impl(std::move(impl))
{
    if (_impl) {
        _impl->setListener(this);
    }
}

SessionWrapper::~SessionWrapper()
{
    disconnectFromSfu();
    if (_impl) {
        _impl->setListener(nullptr);
    }
}

bool SessionWrapper::connectToSfu(const QString& url, const QString& token)
{
    return _impl && _impl->connect(url.toStdString(), token.toStdString());
}

bool SessionWrapper::connecting() const
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

SessionWrapper::State SessionWrapper::state() const
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

QString SessionWrapper::sid() const
{
    if (_impl) {
        return QString::fromStdString(_impl->sid());
    }
    return {};
}

QString SessionWrapper::identity() const
{
    if (_impl) {
        return QString::fromStdString(_impl->identity());
    }
    return {};
}

QString SessionWrapper::name() const
{
    if (_impl) {
        return QString::fromStdString(_impl->name());
    }
    return {};
}

void SessionWrapper::disconnectFromSfu()
{
    if (_impl) {
        _impl->disconnect();
    }
}

bool SessionWrapper::sendChatMessage(const QString& message)
{
    return _impl && _impl->sendChatMessage(message.toStdString());
}

bool SessionWrapper::addMicrophoneTrack()
{
    if (_impl) {
        if (!_micTrack) {
            _micTrack = _impl->addMicrophoneTrack();
        }
        return nullptr != _micTrack;
    }
    return false;
}

bool SessionWrapper::addCameraTrack()
{
    if (_impl) {
        if (!_cameraTrack) {
            _cameraTrack = _impl->addCameraTrack();
        }
        return nullptr != _cameraTrack;
    }
    return false;
}

void SessionWrapper::removeMicrophoneTrack()
{
    if (_impl && _micTrack) {
        _impl->removeAudioTrack(_micTrack);
        _micTrack = {};
    }
}

void SessionWrapper::removeCameraTrack()
{
    if (_impl && _cameraTrack) {
        _impl->removeVideoTrack(_cameraTrack);
        _cameraTrack = {};
    }
}

void SessionWrapper::muteMicrophoneTrack(bool mute)
{
    if (_micTrack) {
        _micTrack->mute(mute);
    }
}

void SessionWrapper::muteCameraTrack(bool mute)
{
    if (_cameraTrack) {
        _cameraTrack->mute(mute);
    }
}

void SessionWrapper::onError(LiveKitCpp::LiveKitError error, const std::string& what)
{
    emit this->error(QString::fromStdString(LiveKitCpp::toString(error)),
                     QString::fromStdString(what));
}

void SessionWrapper::onChanged(const LiveKitCpp::Participant*)
{
    emit localDataChanged();
}

void SessionWrapper::onStateChanged(LiveKitCpp::SessionState)
{
    emit stateChanged();
}

void SessionWrapper::onChatMessageReceived(const std::string& identity,
                                           const std::string& message, const std::string&,
                                           int64_t, bool deleted, bool)
{
    emit chatMessageReceived(QString::fromStdString(identity), QString::fromStdString(message), deleted);
}
