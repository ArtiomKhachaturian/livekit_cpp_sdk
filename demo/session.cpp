#include "session.h"
#include "demoapp.h"
#include <Service.h>

namespace
{

inline DemoApp* appInstance()
{
    return qobject_cast<DemoApp*>(QCoreApplication::instance());
}

}

Session::Session(std::unique_ptr<LiveKitCpp::Session> impl,
                               QObject *parent)
    : QObject{parent}
    , _impl(std::move(impl))
{
    if (_impl) {
        _impl->setListener(this);
    }
}

Session::~Session()
{
    disconnectFromSfu();
    if (_impl) {
        _impl->setListener(nullptr);
    }
}

bool Session::connectToSfu(const QString& url, const QString& token)
{
    return _impl && _impl->connect(url.toStdString(), token.toStdString());
}

AudioTrackWrapper* Session::addAudioTrack(AudioDevice* device)
{
    if (_impl && device) {
        if (const auto track = _impl->addAudioTrack(device->device())) {
            return new AudioTrackWrapper(track, this);
        }
    }
    return nullptr;
}

CameraTrackWrapper* Session::addCameraTrack(CameraDeviceWrapper* device)
{
    if (_impl && device) {
        if (const auto track = _impl->addCameraTrack(device->device())) {
            return new CameraTrackWrapper(track, this);
        }
    }
    return nullptr;
}

AudioTrackWrapper* Session::addMicrophoneTrack()
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

CameraTrackWrapper* Session::addCameraTrack(const MediaDeviceInfo& info,
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

void Session::destroyAudioTrack(AudioTrackWrapper* track)
{
    if (_impl && track && track->parent() == this) {
        const auto& sdkTrack = track->track();
        if (sdkTrack && !sdkTrack->remote()) {
            _impl->removeAudioTrack(sdkTrack);
        }
        delete track;
    }
}

void Session::destroyVideoTrack(VideoTrackWrapper* track)
{
    if (_impl && track && track->parent() == this) {
        const auto& sdkTrack = track->track();
        if (sdkTrack && !sdkTrack->remote()) {
            _impl->removeVideoTrack(sdkTrack);
        }
        delete track;
    }
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

QString Session::sid() const
{
    if (_impl) {
        return QString::fromStdString(_impl->sid());
    }
    return {};
}

QString Session::identity() const
{
    if (_impl) {
        return QString::fromStdString(_impl->identity());
    }
    return {};
}

QString Session::name() const
{
    if (_impl) {
        return QString::fromStdString(_impl->name());
    }
    return {};
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

void Session::onError(LiveKitCpp::LiveKitError error, const std::string& what)
{
    emit this->error(QString::fromStdString(LiveKitCpp::toString(error)),
                     QString::fromStdString(what));
}

void Session::onChanged(const LiveKitCpp::Participant*)
{
    emit localDataChanged();
}

void Session::onStateChanged(LiveKitCpp::SessionState)
{
    emit stateChanged();
}

void Session::onChatMessageReceived(const std::string& identity,
                                           const std::string& message, const std::string&,
                                           int64_t, bool deleted, bool)
{
    emit chatMessageReceived(QString::fromStdString(identity), QString::fromStdString(message), deleted);
}
