#ifndef LOCALPARTICIPANT_H
#define LOCALPARTICIPANT_H
#include "safeobj.h"
#include "participant.h"
#include "mediadeviceinfo.h"
#include "audiorecordingoptions.h"
#include "localvideotrack.h"
#include "videooptions.h"
#include <QPointer>
#include <QQmlEngine>

namespace LiveKitCpp {
class LocalVideoTrack;
}

class AudioTrack;
class LocalVideoTrack;

class LocalParticipant : public Participant
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalParticipant)
    Q_PROPERTY(bool activeCamera READ activeCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(bool activeSharing READ activeSharing NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(bool activeMicrophone READ activeMicrophone NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(QString cameraTrackId READ cameraTrackId NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(QString sharingTrackId READ sharingTrackId NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(QString microphoneTrackId READ microphoneTrackId NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo sharingDeviceInfo READ sharingDeviceInfo WRITE setSharingDeviceInfo NOTIFY sharingDeviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(VideoOptions sharingOptions READ sharingOptions WRITE setSharingOptions NOTIFY sharingOptionsChanged FINAL)
    Q_PROPERTY(AudioRecordingOptions microphoneOptions READ microphoneOptions WRITE setMicrophoneOptions NOTIFY microphoneOptionsChanged FINAL)
    Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged FINAL)
    Q_PROPERTY(bool sharingMuted READ sharingMuted WRITE setSharingMuted NOTIFY sharingMutedChanged FINAL)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted WRITE setMicrophoneMuted NOTIFY microphoneMutedChanged FINAL)
private:
    template <typename TOptions, class TTrack> struct Element {
        // members
        MediaDeviceInfo _deviceinfo;
        TOptions _options;
        bool _muted = false;
        QPointer<TTrack> _track;
        // getters
        bool trackIsMuted() const { return _track ? _track->muted() : _muted; }
        MediaDeviceInfo trackDeviceInfo() const {
            return _track ? _track->deviceInfo() : _deviceinfo;
        }
        TOptions trackOptions() const {
            if (auto options = getLocalTrackOptions(_track)) {
                return options.value();
            }
            return _options;
        }
        QString trackId() const {
            if (_track) {
                return _track->id();
            }
            return {};
        }
        bool activeTrack() const {
            return nullptr != _track;
        }
    };
    using AudioElement = Element<AudioRecordingOptions, AudioTrack>;
    using VideoElement = Element<VideoOptions, LocalVideoTrack>;
public:
    explicit LocalParticipant(QObject *parent = nullptr);
    ~LocalParticipant() override;
    void setSid(const QString& sid);
    void setIdentity(const QString& identity);
    void setName(const QString& name);
    void activateCamera(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack);
    std::shared_ptr<LiveKitCpp::LocalVideoTrack> deactivateCamera();
    void activateSharing(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack);
    std::shared_ptr<LiveKitCpp::LocalVideoTrack> deactivateSharing();
    void activateMicrophone(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack);
    std::shared_ptr<LiveKitCpp::AudioTrack> deactivateMicrophone();
    bool activeCamera() const { return _sharing.activeTrack(); }
    bool activeSharing() const { return _sharing.activeTrack(); }
    bool activeMicrophone() const { return _microphone.activeTrack(); }
    QString cameraTrackId() const { return _camera.trackId(); }
    QString sharingTrackId() const { return _sharing.trackId(); }
    QString microphoneTrackId() const { return _microphone.trackId(); }
    MediaDeviceInfo cameraDeviceInfo() const { return _camera.trackDeviceInfo(); }
    MediaDeviceInfo sharingDeviceInfo() const { return _sharing.trackDeviceInfo(); }
    VideoOptions cameraOptions() const { return _camera.trackOptions(); }
    VideoOptions sharingOptions() const { return _sharing.trackOptions(); }
    AudioRecordingOptions microphoneOptions() const { return _microphone.trackOptions(); }
    bool cameraMuted() const { return _camera.trackIsMuted(); }
    bool sharingMuted() const { return _sharing.trackIsMuted(); }
    bool microphoneMuted() const { return _microphone.trackIsMuted(); }
    // overrides of Participant
    QString sid() const final { return _sid; }
    QString identity() const final { return _identity; }
    QString name() const final { return _name; }
public slots:
    void setCameraDeviceInfo(const MediaDeviceInfo& info = {});
    void setSharingDeviceInfo(const MediaDeviceInfo& info);
    void setCameraOptions(const VideoOptions& options);
    void setSharingOptions(const VideoOptions& options);
    void setMicrophoneOptions(const AudioRecordingOptions& options);
    void setCameraMuted(bool muted);
    void setSharingMuted(bool muted);
    void setMicrophoneMuted(bool muted);
signals:
    void activeCameraChanged();
    void activeSharingChanged();
    void activeMicrophoneChanged();
    void cameraDeviceInfoChanged();
    void cameraOptionsChanged();
    void cameraMutedChanged();
    void sharingMutedChanged();
    void sharingOptionsChanged();
    void sharingDeviceInfoChanged();
    void microphoneMutedChanged();
    void microphoneOptionsChanged();
private slots:
    void onCameraDeviceInfoChanged();
    void onCameraOptionsChanged();
    void onCameraMuted();
    void onSharingDeviceInfoChanged();
    void onSharingMuted();
    void onMicrophoneMuted();
private:
    template <class TElement, typename TSignal>
    void setMuted(TElement* element, bool muted, TSignal signal);
    template <class TElement, typename TSignal>
    void setDeviceInfo(TElement* element, const MediaDeviceInfo& info, TSignal signal);
    template <class TElement, class TOptions, typename TSignal>
    void setOptions(TElement* element, const TOptions& options, TSignal signal);
    static void setLocalTrackOptions(const QPointer<LocalVideoTrack>& track, const VideoOptions& options);
    static std::optional<VideoOptions> getLocalTrackOptions(const QPointer<LocalVideoTrack>& track);
    // not yet implemented
    static void setLocalTrackOptions(const QPointer<AudioTrack>& /*track*/, const AudioRecordingOptions& /*options*/) {}
    static std::optional<AudioRecordingOptions> getLocalTrackOptions(const QPointer<AudioTrack>& /*track*/) { return std::nullopt; }
private:
    AudioElement _microphone;
    VideoElement _camera;
    VideoElement _sharing;
    SafeObj<QString> _sid;
    SafeObj<QString> _identity;
    SafeObj<QString> _name;
};

#endif // LOCALPARTICIPANT_H
