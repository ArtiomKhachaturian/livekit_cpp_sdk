#ifndef Session_H
#define Session_H
#include "localparticipant.h"
#include "remoteparticipant.h"
#include <livekit/rtc/SessionListener.h>
#include <livekit/rtc/RemoteParticipantListener.h>
#include <livekit/rtc/Session.h>
#include <QObject>
#include <QQmlEngine>
#include <QHash>
#include <memory>

namespace LiveKitCpp {
class Service;
}

class Session : public QObject, private LiveKitCpp::SessionListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Session)
public:
    enum State
    {
        TransportConnecting,
        TransportConnected,
        TransportDisconnecting,
        TransportDisconnected,
        RtcConnecting,
        RtcConnected,
        RtcDisconnected,
        RtcClosed
    };
public:
    Q_ENUM(State)
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(bool connecting READ connecting NOTIFY stateChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(bool activeCamera READ activeCamera WRITE setActiveCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(bool activeMicrophone READ activeMicrophone WRITE setActiveMicrophone NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(QString cameraTrackId READ cameraTrackId NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(QString microphoneTrackId READ microphoneTrackId NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(CameraOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(LocalParticipant* localParticipant MEMBER _localParticipant CONSTANT)
    Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged FINAL)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted WRITE setMicrophoneMuted NOTIFY microphoneMutedChanged FINAL)
    Q_PROPERTY(QString identity READ identity NOTIFY identityChanged)
public:
    explicit Session(QObject *parent = nullptr);
    ~Session() override;
    bool isValid() const noexcept { return nullptr != _impl; }
    Q_INVOKABLE bool connectToSfu(const QString& url, const QString& token);
    bool activeCamera() const;
    bool activeMicrophone() const;
    QString cameraTrackId() const;
    QString microphoneTrackId() const;
    bool connecting() const;
    bool connected() const;
    State state() const;
    MediaDeviceInfo cameraDeviceInfo() const;
    CameraOptions cameraOptions() const;
    bool cameraMuted() const { return _localParticipant->cameraMuted(); }
    bool microphoneMuted() const { return _localParticipant->microphoneMuted(); }
    QString identity() const { return _localParticipant->identity(); }
public slots:
    void setActiveCamera(bool active);
    void setActiveMicrophone(bool active);
    void setCameraDeviceInfo(const MediaDeviceInfo& info = {});
    void setCameraOptions(const CameraOptions& options);
    void setCameraMuted(bool muted) { _localParticipant->setCameraMuted(muted); }
    void setMicrophoneMuted(bool muted) { _localParticipant->setMicrophoneMuted(muted); }
    Q_INVOKABLE void disconnectFromSfu();
    Q_INVOKABLE bool sendChatMessage(const QString& message);
signals:
    void chatMessageReceived(const QString& participantIdentity,
                             const QString& message, bool deleted);
    void error(const QString& desc, const QString& details = {});
    void stateChanged();
    void activeCameraChanged();
    void activeMicrophoneChanged();
    void cameraDeviceInfoChanged();
    void cameraOptionsChanged();
    void cameraMutedChanged();
    void microphoneMutedChanged();
    void identityChanged();
    void remoteParticipantAdded(RemoteParticipant* participant);
    void remoteParticipantRemoved(RemoteParticipant* participant);
private slots:
    void addRemoteParticipant(const QString& sid);
    void removeRemoteParticipant(const QString& sid);
private:
    static std::unique_ptr<LiveKitCpp::Session> create();
    // impl. of SessionListener
    void onError(LiveKitCpp::LiveKitError error, const std::string& what) final;
    void onChanged(const LiveKitCpp::Participant*) final;
    void onSpeakerInfoChanged(const LiveKitCpp::Participant* participant,
                              float level, bool active) final;
    void onStateChanged(LiveKitCpp::SessionState) final;
    void onChatMessageReceived(const LiveKitCpp::ChatMessage& message,
                               const std::string& participantIdentity,
                               const std::vector<std::string>&) final;
    void onRemoteParticipantAdded(const std::string& sid) final;
    void onRemoteParticipantRemoved(const std::string& sid) final;
private:
    const std::unique_ptr<LiveKitCpp::Session> _impl;
    LocalParticipant* const _localParticipant;
    QHash<QString, RemoteParticipant*> _remoteParticipants;
};

#endif // Session_H
