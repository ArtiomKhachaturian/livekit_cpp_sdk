#include "localparticipant.h"
#include "demoapp.h"
#include <livekit/rtc/Service.h>
#include <livekit/rtc/media/LocalVideoTrack.h>
#include <QThread>
#include <optional>

LocalParticipant::LocalParticipant(QObject *parent)
    : Participant(parent)
{
    _camera._options = LiveKitCpp::Service::defaultCameraOptions();
}

LocalParticipant::~LocalParticipant()
{
    disposeCamera();
    disposeSharing();
    disposeMicrpohone();
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

void LocalParticipant::setCameraTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack)
{
    if (sdkTrack) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, &LocalParticipant::setCameraTrack, sdkTrack);
        }
        else if (!_camera._track && _camera._id.toStdString() == sdkTrack->id()) {
            _camera._track = addVideo(sdkTrack);
            if (_camera._track) {
                _camera._track->setFilter(_videoFilter);
                _camera._track->setDeviceInfo(_camera._deviceinfo);
                _camera._track->setMuted(_camera._muted);
                QObject::connect(_camera._track, &VideoTrack::deviceInfoChanged,
                                 this, &LocalParticipant::onCameraDeviceInfoChanged);
                QObject::connect(_camera._track, &VideoTrack::optionsChanged,
                                 this, &LocalParticipant::onCameraOptionsChanged);
                QObject::connect(_camera._track, &VideoTrack::muteChanged,
                                 this, &LocalParticipant::onCameraMuted);
            }
        }
    }
}

void LocalParticipant::setSharingTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack)
{
    if (sdkTrack) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, &LocalParticipant::setSharingTrack, sdkTrack);
        }
        else if (!_sharing._track && _sharing._id.toStdString() == sdkTrack->id()) {
            _sharing._track = addVideo(sdkTrack);
            if (_sharing._track) {
                _sharing._track->setFilter(_videoFilter);
                _sharing._track->setDeviceInfo(_sharing._deviceinfo);
                _sharing._track->setMuted(_sharing._muted);
                QObject::connect(_sharing._track, &VideoTrack::deviceInfoChanged,
                                 this, &LocalParticipant::onSharingDeviceInfoChanged);
                QObject::connect(_sharing._track, &VideoTrack::muteChanged,
                                 this, &LocalParticipant::onSharingMuted);
            }
        }
    }
}

void LocalParticipant::setMicrophoneTrack(const std::shared_ptr<LiveKitCpp::LocalAudioTrack>& sdkTrack)
{
    if (sdkTrack) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, &LocalParticipant::setMicrophoneTrack, sdkTrack);
        }
        else if (!_microphone._track && _microphone._id.toStdString() == sdkTrack->id()) {
            _microphone._track = addAudio(sdkTrack);
            if (_microphone._track) {
                _microphone._track->setMuted(_microphone._muted);
                QObject::connect(_microphone._track, &AudioTrack::muteChanged,
                                 this, &LocalParticipant::onMicrophoneMuted);
            }
        }
    }
}

void LocalParticipant::activateCamera(const QString& id)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::activateCamera, id);
    }
    else if (id != _camera._id) {
        disposeCamera();
        _camera._id = id;
        emit activeCameraChanged();
    }
}

void LocalParticipant::activateSharing(const QString& id)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::activateSharing, id);
    }
    else if (id != _sharing._id) {
        disposeSharing();
        _sharing._id = id;
        emit activeSharingChanged();
    }
}

void LocalParticipant::activateMicrophone(const QString& id)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::activateMicrophone, id);
    }
    else if (id != _microphone._id) {
        disposeMicrpohone();
        _microphone._id = id;
        emit activeMicrophoneChanged();
    }
}

void LocalParticipant::setCameraDeviceInfo(const MediaDeviceInfo& info)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setCameraDeviceInfo, info);
    }
    else {
        setDeviceInfo(&_camera, info, &LocalParticipant::cameraDeviceInfoChanged);
    }
}

void LocalParticipant::setSharingDeviceInfo(const MediaDeviceInfo& info)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setSharingDeviceInfo, info);
    }
    else {
        setDeviceInfo(&_sharing, info, &LocalParticipant::sharingDeviceInfoChanged);
    }
}

void LocalParticipant::setCameraOptions(const VideoOptions& options)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setCameraOptions, options);
    }
    else {
        setOptions(&_camera, options, &LocalParticipant::cameraOptionsChanged);
    }
}

void LocalParticipant::setSharingOptions(const VideoOptions& options)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setSharingOptions, options);
    }
    else {
        setOptions(&_sharing, options, &LocalParticipant::sharingOptionsChanged);
    }
}

void LocalParticipant::setMicrophoneOptions(const AudioRecordingOptions& options)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setMicrophoneOptions, options);
    }
    else {
        setOptions(&_microphone, options, &LocalParticipant::microphoneMutedChanged);
    }
}

void LocalParticipant::setCameraMuted(bool muted)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setCameraMuted, muted);
    }
    else {
        setMuted(&_camera, muted, &LocalParticipant::cameraMutedChanged);
    }
}

void LocalParticipant::setSharingMuted(bool muted)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setSharingMuted, muted);
    }
    else {
        setMuted(&_sharing, muted, &LocalParticipant::sharingMutedChanged);
    }
}

void LocalParticipant::setMicrophoneMuted(bool muted)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setMicrophoneMuted, muted);
    }
    else {
        setMuted(&_microphone, muted, &LocalParticipant::microphoneMutedChanged);
    }
}

void LocalParticipant::setVideoFilter(const QString& filter)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, &LocalParticipant::setVideoFilter, filter);
    }
    else if (filter != _videoFilter) {
        _videoFilter = filter;
        if (_camera._track) {
            _camera._track->setFilter(filter);
        }
        if (_sharing._track) {
            _sharing._track->setFilter(filter);
        }
        emit videoFilterChanged();
    }
}

void LocalParticipant::onCameraDeviceInfoChanged()
{
    setDeviceInfo(&_camera, _camera.trackDeviceInfo(), &LocalParticipant::cameraDeviceInfoChanged);
}

void LocalParticipant::onCameraOptionsChanged()
{
    setOptions(&_camera, _camera.trackOptions(), &LocalParticipant::cameraOptionsChanged);
}

void LocalParticipant::onCameraMuted()
{
    setMuted(&_camera, _camera.trackIsMuted(), &LocalParticipant::cameraMutedChanged);
}

void LocalParticipant::onSharingDeviceInfoChanged()
{
    setDeviceInfo(&_sharing, _sharing.trackDeviceInfo(), &LocalParticipant::sharingDeviceInfoChanged);
}

void LocalParticipant::onSharingMuted()
{
    setMuted(&_sharing, _sharing.trackIsMuted(), &LocalParticipant::sharingMutedChanged);
}

void LocalParticipant::onMicrophoneMuted()
{
    setMuted(&_microphone, _microphone.trackIsMuted(), &LocalParticipant::microphoneMutedChanged);
}

void LocalParticipant::disposeCamera()
{
    if (_camera._track) {
        _camera._track->disconnect(this);
        if (removeVideoTrack(_camera._id)) {
            _camera._track = nullptr;
        }
    }
}

void LocalParticipant::disposeSharing()
{
    if (_sharing._track) {
        _sharing._track->disconnect(this);
        if (removeVideoTrack(_sharing._id)) {
            _sharing._track = nullptr;
        }
    }
}

void LocalParticipant::disposeMicrpohone()
{
    if (_microphone._track) {
        _microphone._track->disconnect(this);
        if (removeAudioTrack(_microphone._id)) {
            _microphone._track = nullptr;
        }
    }
}

template <class TElement, typename TSignal>
void LocalParticipant::setMuted(TElement* element, bool muted, TSignal signal)
{
    if (element && element->_muted != muted) {
        element->_muted = muted;
        if (element->_track) {
            element->_track->setMuted(muted);
        }
        else {
            emit ((*this).*signal)();
        }
    }
}

template <class TElement, typename TSignal>
void LocalParticipant::setDeviceInfo(TElement* element, const MediaDeviceInfo& info, TSignal signal)
{
    if (element && element->_deviceinfo != info) {
        element->_deviceinfo = info;
        if (element->_track) {
            element->_track->setDeviceInfo(info);
        }
        else {
            emit ((*this).*signal)();
        }
    }
}

template <class TElement, class TOptions, typename TSignal>
void LocalParticipant::setOptions(TElement* element, const TOptions& options, TSignal signal)
{
    if (element && element->_options != options) {
        if (element->_track) {
            setTrackOptions(element->_track, options);
        }
        else {
            emit ((*this).*signal)();
        }
    }
}

void LocalParticipant::setTrackOptions(const QPointer<VideoTrack>& track, const VideoOptions& options)
{
    if (track) {
        track->setOptions(options);
    }
}

std::optional<VideoOptions> LocalParticipant::getTrackOptions(const QPointer<VideoTrack>& track)
{
    if (track) {
        return track->options();
    }
    return std::nullopt;
}
