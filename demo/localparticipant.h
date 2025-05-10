#ifndef LOCALPARTICIPANT_H
#define LOCALPARTICIPANT_H
#include "safeobj.h"
#include "participant.h"
#include "mediadeviceinfo.h"
#include "audiorecordingoptions.h"
#include "audiotrack.h"
#include "videotrack.h"
#include "videooptions.h"
#include <QPointer>
#include <QQmlEngine>

namespace LiveKitCpp {
class LocalAudioTrack;
class LocalVideoTrack;
}


class LocalParticipant : public Participant
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalParticipant)
    Q_PROPERTY(bool activeCamera READ activeCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(bool activeSharing READ activeSharing NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(bool activeMicrophone READ activeMicrophone NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(QString cameraTrackId READ cameraTrackId WRITE activateCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(QString sharingTrackId READ sharingTrackId WRITE activateSharing NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(QString microphoneTrackId READ microphoneTrackId WRITE activateMicrophone NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo sharingDeviceInfo READ sharingDeviceInfo WRITE setSharingDeviceInfo NOTIFY sharingDeviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(VideoOptions sharingOptions READ sharingOptions WRITE setSharingOptions NOTIFY sharingOptionsChanged FINAL)
    Q_PROPERTY(AudioRecordingOptions microphoneOptions READ microphoneOptions WRITE setMicrophoneOptions NOTIFY microphoneOptionsChanged FINAL)
    Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged FINAL)
    Q_PROPERTY(bool sharingMuted READ sharingMuted WRITE setSharingMuted NOTIFY sharingMutedChanged FINAL)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted WRITE setMicrophoneMuted NOTIFY microphoneMutedChanged FINAL)
    Q_PROPERTY(QString videoFilter READ videoFilter WRITE setVideoFilter NOTIFY videoFilterChanged FINAL)
private:
    template <typename TOptions, class TTrack> struct Element {
        // members
        MediaDeviceInfo _deviceinfo;
        TOptions _options;
        bool _muted = false;
        QString _id;
        QPointer<TTrack> _track;
        // getters
        bool trackIsMuted() const { return _track ? _track->isMuted() : _muted; }
        MediaDeviceInfo trackDeviceInfo() const {
            return _track ? _track->deviceInfo() : _deviceinfo;
        }
        TOptions trackOptions() const {
            if (auto options = getTrackOptions(_track)) {
                return options.value();
            }
            return _options;
        }
        bool active() const { return !_id.isEmpty(); }
    };
    using AudioElement = Element<AudioRecordingOptions, AudioTrack>;
    using VideoElement = Element<VideoOptions, VideoTrack>;
public:
    explicit LocalParticipant(QObject *parent = nullptr);
    ~LocalParticipant() override;
    void setSid(const QString& sid);
    void setIdentity(const QString& identity);
    void setName(const QString& name);
    void setCameraTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack);
    void setSharingTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& sdkTrack);
    void setMicrophoneTrack(const std::shared_ptr<LiveKitCpp::LocalAudioTrack>& sdkTrack);
    bool activeCamera() const { return _sharing.active(); }
    bool activeSharing() const { return _sharing.active(); }
    bool activeMicrophone() const { return _microphone.active(); }
    QString cameraTrackId() const { return _camera._id; }
    QString sharingTrackId() const { return _sharing._id; }
    QString microphoneTrackId() const { return _microphone._id; }
    MediaDeviceInfo cameraDeviceInfo() const { return _camera.trackDeviceInfo(); }
    MediaDeviceInfo sharingDeviceInfo() const { return _sharing.trackDeviceInfo(); }
    VideoOptions cameraOptions() const { return _camera.trackOptions(); }
    VideoOptions sharingOptions() const { return _sharing.trackOptions(); }
    AudioRecordingOptions microphoneOptions() const { return _microphone.trackOptions(); }
    bool cameraMuted() const { return _camera.trackIsMuted(); }
    bool sharingMuted() const { return _sharing.trackIsMuted(); }
    bool microphoneMuted() const { return _microphone.trackIsMuted(); }
    const auto& videoFilter() const noexcept { return _videoFilter; }
    // overrides of Participant
    QString sid() const final { return _sid; }
    QString identity() const final { return _identity; }
    QString name() const final { return _name; }
public slots:
    void activateCamera(const QString& id);
    void activateSharing(const QString& id);
    void activateMicrophone(const QString& id);
    void deactivateCamera() { activateCamera({}); }
    void deactivateSharing() { activateSharing({}); }
    void deactivateMicrophone() { activateMicrophone({}); }
    void setCameraDeviceInfo(const MediaDeviceInfo& info = {});
    void setSharingDeviceInfo(const MediaDeviceInfo& info);
    void setCameraOptions(const VideoOptions& options);
    void setSharingOptions(const VideoOptions& options);
    void setMicrophoneOptions(const AudioRecordingOptions& options);
    void setCameraMuted(bool muted);
    void setSharingMuted(bool muted);
    void setMicrophoneMuted(bool muted);
    void setVideoFilter(const QString& filter);
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
    void videoFilterChanged();
private slots:
    void onCameraDeviceInfoChanged();
    void onCameraOptionsChanged();
    void onCameraMuted();
    void onSharingDeviceInfoChanged();
    void onSharingMuted();
    void onMicrophoneMuted();
private:
    void disposeCamera();
    void disposeSharing();
    void disposeMicrpohone();
    template <class TElement, typename TSignal>
    void setMuted(TElement* element, bool muted, TSignal signal);
    template <class TElement, typename TSignal>
    void setDeviceInfo(TElement* element, const MediaDeviceInfo& info, TSignal signal);
    template <class TElement, class TOptions, typename TSignal>
    void setOptions(TElement* element, const TOptions& options, TSignal signal);
    static void setTrackOptions(const QPointer<VideoTrack>& track, const VideoOptions& options);
    static std::optional<VideoOptions> getTrackOptions(const QPointer<VideoTrack>& track);
    // not yet implemented
    static void setTrackOptions(const QPointer<AudioTrack>& /*track*/, const AudioRecordingOptions& /*options*/) {}
    static std::optional<AudioRecordingOptions> getTrackOptions(const QPointer<AudioTrack>& /*track*/) { return std::nullopt; }
private:
    AudioElement _microphone;
    VideoElement _camera;
    VideoElement _sharing;
    QString _videoFilter;
    SafeObj<QString> _sid;
    SafeObj<QString> _identity;
    SafeObj<QString> _name;
};

#endif // LOCALPARTICIPANT_H
