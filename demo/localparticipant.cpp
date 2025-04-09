#include "localparticipant.h"
#include "cameratrack.h"
#include <livekit/Service.h>
#include <livekit/media/CameraTrack.h>

LocalParticipant::LocalParticipant(QObject *parent)
    : Participant(parent)
    , _cameraOptions(CameraOptions::defaultOptions())
{
}

LocalParticipant::~LocalParticipant()
{
    deactivateCamera();
    deactivateMicrophone();
}

void LocalParticipant::setSid(const QString& sid)
{
    if (_sid.exchange(sid)) {
        emit sidChanged();
    }
}

void LocalParticipant::setIdentity(const QString& identity)
{
    if (_identity.exchange(identity)) {
        emit identityChanged();
    }
}

void LocalParticipant::setName(const QString& name)
{
    if (_name.exchange(name)) {
        emit nameChanged();
    }
}

void LocalParticipant::activateCamera(const std::shared_ptr<LiveKitCpp::CameraTrack>& sdkTrack)
{
    if (sdkTrack && !_camera) {
        _camera = addVideo<CameraTrack>(sdkTrack);
        if (_camera) {
            _camera->setMuted(_cameraMuted);
            QObject::connect(_camera, &CameraTrack::deviceInfoChanged,
                             this, &LocalParticipant::onCameraDeviceInfoChanged);
            QObject::connect(_camera, &CameraTrack::optionsChanged,
                             this, &LocalParticipant::onCameraOptionsChanged);
            QObject::connect(_camera, &VideoTrack::muteChanged,
                             this, &LocalParticipant::onCameraMuted);
            emit activeCameraChanged();
        }
    }
}

std::shared_ptr<LiveKitCpp::VideoTrack> LocalParticipant::deactivateCamera()
{
    std::shared_ptr<LiveKitCpp::VideoTrack> track;
    if (_camera) {
        _camera->disconnect(this);
        track = removeVideoTrack(_camera->id());
        if (track) {
            _camera = nullptr;
            emit activeCameraChanged();
        }
    }
    return track;
}

void LocalParticipant::activateMicrophone(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack)
{
    if (sdkTrack && !_microphone) {
        _microphone = addAudio<>(sdkTrack);
        if (_microphone) {
            _microphone->setMuted(_microphoneMuted);
            QObject::connect(_microphone, &AudioTrack::muteChanged,
                             this, &LocalParticipant::onMicrophoneMuted);
            emit activeMicrophoneChanged();
        }
    }
}

std::shared_ptr<LiveKitCpp::AudioTrack> LocalParticipant::deactivateMicrophone()
{
    std::shared_ptr<LiveKitCpp::AudioTrack> track;
    if (_microphone) {
        _microphone->disconnect(this);
        track = removeAudioTrack(_microphone->id());
        if (track) {
            _microphone = nullptr;
            emit activeMicrophoneChanged();
        }
    }
    return track;
}

bool LocalParticipant::activeCamera() const
{
    return nullptr != _camera;
}

bool LocalParticipant::activeMicrophone() const
{
    return nullptr != _microphone;
}

QString LocalParticipant::cameraTrackId() const
{
    if (_camera) {
        return _camera->id();
    }
    return {};
}

QString LocalParticipant::microphoneTrackId() const
{
    if (_microphone) {
        return _microphone->id();
    }
    return {};
}

MediaDeviceInfo LocalParticipant::cameraDeviceInfo() const
{
    return _camera ? _camera->deviceInfo() : _cameraDeviceinfo;
}

CameraOptions LocalParticipant::cameraOptions() const
{
    return _camera ? _camera->options() : _cameraOptions;
}

bool LocalParticipant::cameraMuted() const
{
    return _camera ? _camera->muted() : _cameraMuted;
}

bool LocalParticipant::microphoneMuted() const
{
    return _microphone ? _microphone->muted() : _microphoneMuted;
}

void LocalParticipant::setCameraDeviceInfo(const MediaDeviceInfo& info)
{
    if (_camera) {
        _cameraDeviceinfo = info;
        _camera->setDeviceInfo(info);
    }
    else {
        changeCameraDeviceInfo(info);
    }
}

void LocalParticipant::setCameraOptions(const CameraOptions& options)
{
    if (_camera) {
        _cameraOptions = options;
        _camera->setOptions(options);
    }
    else  {
        changeCameraOptions(options);
    }
}

void LocalParticipant::setCameraMuted(bool muted)
{
    if (_camera) {
        _cameraMuted = muted;
        _camera->setMuted(muted);
    }
    else {
        changeCameraMuted(muted);
    }
}

void LocalParticipant::setMicrophoneMuted(bool muted)
{
    if (_microphone) {
        _microphoneMuted = muted;
        _microphone->setMuted(muted);
    }
    else {
        changeMicrophoneMuted(muted);
    }
}

void LocalParticipant::onCameraDeviceInfoChanged()
{
    if (_camera) {
        changeCameraDeviceInfo(_camera->deviceInfo());
    }
}

void LocalParticipant::onCameraOptionsChanged()
{
    if (_camera) {
        changeCameraOptions(_camera->options());
    }
}

void LocalParticipant::onCameraMuted()
{
    if (_camera) {
        changeCameraMuted(_camera->muted());
    }
}

void LocalParticipant::onMicrophoneMuted()
{
    if (_microphone) {
        changeMicrophoneMuted(_microphone->muted());
    }
}

void LocalParticipant::changeCameraDeviceInfo(const MediaDeviceInfo& info)
{
    if (_cameraDeviceinfo != info) {
        _cameraDeviceinfo = info;
        emit cameraDeviceInfoChanged();
    }
}

void LocalParticipant::changeCameraOptions(const CameraOptions& options)
{
    if (_cameraOptions != options) {
        _cameraOptions = options;
        emit cameraOptionsChanged();
    }
}

void LocalParticipant::changeCameraMuted(bool muted)
{
    if (_cameraMuted != muted) {
        _cameraMuted = muted;
        emit cameraMutedChanged();
    }
}

void LocalParticipant::changeMicrophoneMuted(bool muted)
{
    if (_microphoneMuted != muted) {
        _microphoneMuted = muted;
        emit microphoneMutedChanged();
    }
}
