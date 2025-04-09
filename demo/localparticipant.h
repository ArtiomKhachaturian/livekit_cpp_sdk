#ifndef LOCALPARTICIPANT_H
#define LOCALPARTICIPANT_H
#include "safeobj.h"
#include "participant.h"
#include "mediadeviceinfo.h"
#include "cameraoptions.h"
#include <QPointer>
#include <QQmlEngine>

namespace LiveKitCpp {
class CameraTrack;
}

class AudioTrack;
class CameraTrack;

class LocalParticipant : public Participant
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalParticipant)
    Q_PROPERTY(bool activeCamera READ activeCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(bool activeMicrophone READ activeMicrophone NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(QString cameraTrackId READ cameraTrackId NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(QString microphoneTrackId READ microphoneTrackId NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(CameraOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged FINAL)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted WRITE setMicrophoneMuted NOTIFY microphoneMutedChanged FINAL)
public:
    explicit LocalParticipant(QObject *parent = nullptr);
    ~LocalParticipant() override;
    void setSid(const QString& sid);
    void setIdentity(const QString& identity);
    void setName(const QString& name);
    void activateCamera(const std::shared_ptr<LiveKitCpp::CameraTrack>& sdkTrack);
    std::shared_ptr<LiveKitCpp::VideoTrack> deactivateCamera();
    void activateMicrophone(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack);
    std::shared_ptr<LiveKitCpp::AudioTrack> deactivateMicrophone();
    bool activeCamera() const;
    bool activeMicrophone() const;
    QString cameraTrackId() const;
    QString microphoneTrackId() const;
    MediaDeviceInfo cameraDeviceInfo() const;
    CameraOptions cameraOptions() const;
    bool cameraMuted() const;
    bool microphoneMuted() const;
    // overrides of Participant
    QString sid() const final { return _sid; }
    QString identity() const final { return _identity; }
    QString name() const final { return _name; }
public slots:
    void setCameraDeviceInfo(const MediaDeviceInfo& info = {});
    void setCameraOptions(const CameraOptions& options);
    void setCameraMuted(bool muted);
    void setMicrophoneMuted(bool muted);
signals:
    void activeCameraChanged();
    void activeMicrophoneChanged();
    void cameraDeviceInfoChanged();
    void cameraOptionsChanged();
    void cameraMutedChanged();
    void microphoneMutedChanged();
private slots:
    void onCameraDeviceInfoChanged();
    void onCameraOptionsChanged();
    void onCameraMuted();
    void onMicrophoneMuted();
private:
    void changeCameraDeviceInfo(const MediaDeviceInfo& info);
    void changeCameraOptions(const CameraOptions& options);
    void changeCameraMuted(bool muted);
    void changeMicrophoneMuted(bool muted);
private:
    MediaDeviceInfo _cameraDeviceinfo;
    CameraOptions _cameraOptions;
    bool _microphoneMuted = false;
    bool _cameraMuted = false;
    SafeObj<QString> _sid;
    SafeObj<QString> _identity;
    SafeObj<QString> _name;
    QPointer<CameraTrack> _camera;
    QPointer<AudioTrack> _microphone;
};

Q_DECLARE_METATYPE(LocalParticipant*)

#endif // LOCALPARTICIPANT_H
