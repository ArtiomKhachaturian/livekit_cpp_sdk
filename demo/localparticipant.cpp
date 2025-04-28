#include "localparticipant.h"
#include "demoapp.h"
#include <livekit/rtc/Service.h>
#include <livekit/rtc/media/LocalVideoTrack.h>
#include <optional>

LocalParticipant::LocalParticipant(QObject *parent)
    : Participant(parent)
{
    _camera._options = LiveKitCpp::Service::defaultCameraOptions();
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

void LocalParticipant::activateCamera(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack)
{
    if (sdkTrack && !_camera._track) {
        _camera._track = addVideo<LocalVideoTrack>(sdkTrack);
        if (_camera._track) {
            _camera._track->setFilter(_videoFilter);
            _camera._track->setDeviceInfo(_camera._deviceinfo);
            _camera._track->setMuted(_camera._muted);
            QObject::connect(_camera._track, &LocalVideoTrack::deviceInfoChanged,
                             this, &LocalParticipant::onCameraDeviceInfoChanged);
            QObject::connect(_camera._track, &LocalVideoTrack::optionsChanged,
                             this, &LocalParticipant::onCameraOptionsChanged);
            QObject::connect(_camera._track, &VideoTrack::muteChanged,
                             this, &LocalParticipant::onCameraMuted);
            emit activeCameraChanged();
        }
    }
}

std::shared_ptr<LiveKitCpp::LocalVideoTrack> LocalParticipant::deactivateCamera()
{
    std::shared_ptr<LiveKitCpp::VideoTrack> track;
    if (_camera._track) {
        _camera._track->disconnect(this);
        track = removeVideoTrack(_camera._track->id());
        if (track) {
            _camera._track = nullptr;
            emit activeCameraChanged();
        }
    }
    return std::dynamic_pointer_cast<LiveKitCpp::LocalVideoTrack>(track);
}

void LocalParticipant::activateSharing(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack)
{
    if (sdkTrack && !_sharing._track) {
        _sharing._track = addVideo<LocalVideoTrack>(sdkTrack);
        if (_sharing._track) {
            _sharing._track->setFilter(_videoFilter);
            _sharing._track->setDeviceInfo(_sharing._deviceinfo);
            _sharing._track->setMuted(_sharing._muted);
            QObject::connect(_sharing._track, &LocalVideoTrack::deviceInfoChanged,
                             this, &LocalParticipant::onSharingDeviceInfoChanged);
            QObject::connect(_sharing._track, &VideoTrack::muteChanged,
                             this, &LocalParticipant::onSharingMuted);
            emit activeSharingChanged();
        }
    }
}

std::shared_ptr<LiveKitCpp::LocalVideoTrack> LocalParticipant::deactivateSharing()
{
    std::shared_ptr<LiveKitCpp::VideoTrack> track;
    if (_sharing._track) {
        _sharing._track->disconnect(this);
        track = removeVideoTrack(_sharing._track->id());
        if (track) {
            _sharing._track = nullptr;
            emit activeSharingChanged();
        }
    }
    return std::dynamic_pointer_cast<LiveKitCpp::LocalVideoTrack>(track);
}

void LocalParticipant::activateMicrophone(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack)
{
    if (sdkTrack && !_microphone._track) {
        _microphone._track = addAudio<>(sdkTrack);
        if (_microphone._track) {
            _microphone._track->setMuted(_microphone._muted);
            QObject::connect(_microphone._track, &AudioTrack::muteChanged,
                             this, &LocalParticipant::onMicrophoneMuted);
            emit activeMicrophoneChanged();
        }
    }
}

std::shared_ptr<LiveKitCpp::AudioTrack> LocalParticipant::deactivateMicrophone()
{
    std::shared_ptr<LiveKitCpp::AudioTrack> track;
    if (_microphone._track) {
        _microphone._track->disconnect(this);
        track = removeAudioTrack(_microphone._track->id());
        if (track) {
            _microphone._track = nullptr;
            emit activeMicrophoneChanged();
        }
    }
    return track;
}

void LocalParticipant::setCameraDeviceInfo(const MediaDeviceInfo& info)
{
    setDeviceInfo(&_camera, info, &LocalParticipant::cameraDeviceInfoChanged);
}

void LocalParticipant::setSharingDeviceInfo(const MediaDeviceInfo& info)
{
    setDeviceInfo(&_sharing, info, &LocalParticipant::sharingDeviceInfoChanged);
}

void LocalParticipant::setCameraOptions(const VideoOptions& options)
{
    setOptions(&_camera, options, &LocalParticipant::cameraOptionsChanged);
}

void LocalParticipant::setSharingOptions(const VideoOptions& options)
{
    setOptions(&_sharing, options, &LocalParticipant::sharingOptionsChanged);
}

void LocalParticipant::setMicrophoneOptions(const AudioRecordingOptions& options)
{
    setOptions(&_microphone, options, &LocalParticipant::microphoneMutedChanged);
}

void LocalParticipant::setCameraMuted(bool muted)
{
    setMuted(&_camera, muted, &LocalParticipant::cameraMutedChanged);
}

void LocalParticipant::setSharingMuted(bool muted)
{
    setMuted(&_sharing, muted, &LocalParticipant::sharingMutedChanged);
}

void LocalParticipant::setMicrophoneMuted(bool muted)
{
    setMuted(&_microphone, muted, &LocalParticipant::microphoneMutedChanged);
}

void LocalParticipant::setVideoFilter(const QString& filter)
{
    if (filter != _videoFilter) {
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
            setLocalTrackOptions(element->_track, options);
        }
        else {
            emit ((*this).*signal)();
        }
    }
}

void LocalParticipant::setLocalTrackOptions(const QPointer<LocalVideoTrack>& track,
                                            const VideoOptions& options)
{
    if (track) {
        track->setOptions(options);
    }
}

std::optional<VideoOptions> LocalParticipant::getLocalTrackOptions(const QPointer<LocalVideoTrack>& track)
{
    if (track) {
        return track->options();
    }
    return std::nullopt;
}
