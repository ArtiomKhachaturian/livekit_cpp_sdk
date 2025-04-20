#ifndef LOCALPARTICIPANT_H
#define LOCALPARTICIPANT_H
#include "safeobj.h"
#include "participant.h"
#include "mediadeviceinfo.h"
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
    Q_PROPERTY(bool activeMicrophone READ activeMicrophone NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(QString cameraTrackId READ cameraTrackId NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(QString microphoneTrackId READ microphoneTrackId NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged FINAL)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted WRITE setMicrophoneMuted NOTIFY microphoneMutedChanged FINAL)
    Q_PROPERTY(QString sharingTrackId READ sharingTrackId NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo sharingDeviceInfo READ sharingDeviceInfo WRITE setSharingDeviceInfo NOTIFY sharingDeviceInfoChanged FINAL)
    Q_PROPERTY(bool sharingMuted READ sharingMuted WRITE setSharingMuted NOTIFY sharingMutedChanged FINAL)
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
    bool activeCamera() const;
    bool activeSharing() const;
    bool activeMicrophone() const;
    QString cameraTrackId() const;
    QString sharingTrackId() const;
    QString microphoneTrackId() const;
    MediaDeviceInfo cameraDeviceInfo() const;
    VideoOptions cameraOptions() const;
    MediaDeviceInfo sharingDeviceInfo() const;
    bool cameraMuted() const;
    bool sharingMuted() const;
    bool microphoneMuted() const;
    // overrides of Participant
    QString sid() const final { return _sid; }
    QString identity() const final { return _identity; }
    QString name() const final { return _name; }
public slots:
    void setCameraDeviceInfo(const MediaDeviceInfo& info = {});
    void setCameraOptions(const VideoOptions& options);
    void setCameraMuted(bool muted);
    void setSharingDeviceInfo(const MediaDeviceInfo& info);
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
    void sharingDeviceInfoChanged();
    void microphoneMutedChanged();
private slots:
    void onCameraDeviceInfoChanged();
    void onCameraOptionsChanged();
    void onCameraMuted();
    void onSharingDeviceInfoChanged();
    void onSharingMuted();
    void onMicrophoneMuted();
private:
    void changeCameraDeviceInfo(const MediaDeviceInfo& info);
    void changeCameraOptions(const VideoOptions& options);
    void changeCameraMuted(bool muted);
    void changeMicrophoneMuted(bool muted);
    void changeSharingDeviceInfo(const MediaDeviceInfo& info);
    void changeSharingMuted(bool muted);
private:
    MediaDeviceInfo _cameraDeviceinfo;
    VideoOptions _cameraOptions;
    MediaDeviceInfo _sharingDeviceinfo;
    bool _microphoneMuted = false;
    bool _cameraMuted = false;
    bool _sharingMuted = false;
    SafeObj<QString> _sid;
    SafeObj<QString> _identity;
    SafeObj<QString> _name;
    QPointer<LocalVideoTrack> _camera;
    QPointer<LocalVideoTrack> _sharing;
    QPointer<AudioTrack> _microphone;
};

#endif // LOCALPARTICIPANT_H
