#include "sessionwrapper.h"
#include "demoapp.h"
#include <Service.h>

namespace
{

inline DemoApp* appInstance()
{
    return qobject_cast<DemoApp*>(QCoreApplication::instance());
}

}

SessionWrapper::SessionWrapper(std::unique_ptr<LiveKitCpp::Session> impl,
                               QObject *parent)
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

AudioTrackWrapper* SessionWrapper::addAudioTrack(AudioDeviceWrapper* device)
{
    if (_impl && device) {
        if (const auto track = _impl->addAudioTrack(device->device())) {
            return new AudioTrackWrapper(track, this);
        }
    }
    return nullptr;
}

CameraTrackWrapper* SessionWrapper::addCameraTrack(CameraDeviceWrapper* device)
{
    if (_impl && device) {
        if (const auto track = _impl->addCameraTrack(device->device())) {
            return new CameraTrackWrapper(track, this);
        }
    }
    return nullptr;
}

AudioTrackWrapper* SessionWrapper::addMicrophoneTrack()
{
    AudioTrackWrapper* track = nullptr;
    if (const auto app = appInstance()) {
        if (const auto device = app->createMicrophone()) {
            track = addAudioTrack(device);
            device->deleteLater();
        }
    }
    return track;
}

CameraTrackWrapper* SessionWrapper::addCameraTrack(const MediaDeviceInfo& info,
                                                   const CameraOptions& options)
{
    CameraTrackWrapper* track = nullptr;
    if (const auto app = appInstance()) {
        if (const auto device = app->createCamera(info, options)) {
            track = addCameraTrack(device);
            device->deleteLater();
        }
    }
    return track;
}

void SessionWrapper::removeMicrophoneTrack(AudioTrackWrapper* track)
{
    if (_impl && track && track->parent() == this) {
        _impl->removeAudioTrack(track->track());
        delete track;
    }
}

void SessionWrapper::removeCameraTrack(CameraTrackWrapper* track)
{
    if (_impl && track && track->parent() == this) {
        _impl->removeVideoTrack(track->track());
        delete track;
    }
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
