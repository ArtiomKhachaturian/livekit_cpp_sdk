#ifndef Session_H
#define Session_H
#include "localparticipant.h"
#include "remoteparticipant.h"
#include <livekit/rtc/SessionListener.h>
#include <livekit/rtc/RemoteParticipantListener.h>
#include <livekit/rtc/Session.h>
#include <livekit/signaling/sfu/EncryptionType.h>
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
    Q_PROPERTY(bool activeSharing READ activeSharing WRITE setActiveSharing NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(QString cameraTrackId READ cameraTrackId NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(QString sharingTrackId READ sharingTrackId NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(QString microphoneTrackId READ microphoneTrackId NOTIFY activeMicrophoneChanged FINAL)
    Q_PROPERTY(bool cameraMuted READ cameraMuted WRITE setCameraMuted NOTIFY cameraMutedChanged FINAL)
    Q_PROPERTY(bool sharingMuted READ sharingMuted WRITE setSharingMuted NOTIFY sharingMutedChanged FINAL)
    Q_PROPERTY(bool microphoneMuted READ microphoneMuted WRITE setMicrophoneMuted NOTIFY microphoneMutedChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo sharingDeviceInfo READ sharingDeviceInfo WRITE setSharingDeviceInfo NOTIFY sharingDeviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(VideoOptions sharingOptions READ sharingOptions WRITE setSharingOptions NOTIFY sharingOptionsChanged FINAL)
    Q_PROPERTY(AudioRecordingOptions microphoneOptions READ microphoneOptions WRITE setMicrophoneOptions NOTIFY microphoneOptionsChanged FINAL)
    Q_PROPERTY(LocalParticipant* localParticipant MEMBER _localParticipant CONSTANT)
    Q_PROPERTY(QString identity READ identity NOTIFY identityChanged)
public:
    explicit Session(QObject *parent = nullptr);
    ~Session() override;
    bool isValid() const noexcept { return nullptr != _impl; }
    Q_INVOKABLE bool connectToSfu(const QString& url, const QString& token,
                                  bool autoSubscribe, bool adaptiveStream,
                                  bool e2e, const QString& iceTransportPolicy,
                                  const QString& e2ePassPhrase = {});
    bool activeCamera() const { return _activeCamera; }
    bool activeMicrophone() const { return _activeMicrophone; }
    bool activeSharing() const { return _activeSharing; }
    QString cameraTrackId() const;
    QString microphoneTrackId() const;
    QString sharingTrackId() const;
    bool connecting() const;
    bool connected() const;
    State state() const;
    MediaDeviceInfo cameraDeviceInfo() const { return _localParticipant->cameraDeviceInfo(); }
    MediaDeviceInfo sharingDeviceInfo() const { return _localParticipant->sharingDeviceInfo(); }
    VideoOptions cameraOptions() const { return _localParticipant->cameraOptions(); }
    VideoOptions sharingOptions() const { return _localParticipant->sharingOptions(); }
    AudioRecordingOptions microphoneOptions() const { return _localParticipant->microphoneOptions(); }
    bool cameraMuted() const { return _localParticipant->cameraMuted(); }
    bool microphoneMuted() const { return _localParticipant->microphoneMuted(); }
    bool sharingMuted() const { return _localParticipant->sharingMuted(); }
    QString identity() const { return _localParticipant->identity(); }
public slots:
    void setActiveCamera(bool active);
    void setActiveMicrophone(bool active);
    void setActiveSharing(bool active);
    void setCameraDeviceInfo(const MediaDeviceInfo& info = {}) {  _localParticipant->setCameraDeviceInfo(info); }
    void setCameraOptions(const VideoOptions& options) { _localParticipant->setCameraOptions(options); }
    void setCameraMuted(bool muted) { _localParticipant->setCameraMuted(muted); }
    void setMicrophoneMuted(bool muted) { _localParticipant->setMicrophoneMuted(muted); }
    void setSharingDeviceInfo(const MediaDeviceInfo& info) { _localParticipant->setSharingDeviceInfo(info); }
    void setSharingMuted(bool muted) { _localParticipant->setSharingMuted(muted); }
    void setSharingOptions(const VideoOptions& options) { _localParticipant->setSharingOptions(options); }
    void setMicrophoneOptions(const AudioRecordingOptions& options) { _localParticipant->setMicrophoneOptions(options); }
    Q_INVOKABLE void setPrefferedVideoEncoder(const QString& encoder = {});
    Q_INVOKABLE void setPrefferedAudioEncoder(const QString& encoder = {});
    Q_INVOKABLE void disconnectFromSfu();
    Q_INVOKABLE bool sendChatMessage(const QString& message);
signals:
    void chatMessageReceived(const QString& participantIdentity,
                             const QString& message, bool deleted);
    void error(const QString& desc, const QString& details = {});
    void stateChanged();
    void activeCameraChanged();
    void activeMicrophoneChanged();
    void activeSharingChanged();
    void cameraDeviceInfoChanged();
    void cameraOptionsChanged();
    void cameraMutedChanged();
    void microphoneOptionsChanged();
    void microphoneMutedChanged();
    void sharingMutedChanged();
    void sharingDeviceInfoChanged();
    void sharingOptionsChanged();
    void identityChanged();
    void remoteParticipantAdded(RemoteParticipant* participant);
    void remoteParticipantRemoved(RemoteParticipant* participant);
    void localMediaTrackAddFailure(bool audio, const QString& id, const QString& details = {});
private slots:
    void addRemoteParticipant(const QString& sid);
    void removeRemoteParticipant(const QString& sid);
private:
    static std::unique_ptr<LiveKitCpp::Session> create(LiveKitCpp::Options options);
    void setSessionImpl(std::unique_ptr<LiveKitCpp::Session> impl);
    void resetSessionImpl() { setSessionImpl(nullptr); }
    void addCameraTrack();
    void addMicrophoneTrack();
    void addSharingTrack();
    void removeCameraTrack();
    void removeMicrophoneTrack();
    void removeSharingTrack();
    // impl. of SessionListener
    void onLocalAudioTrackAdded(const std::shared_ptr<LiveKitCpp::LocalAudioTrack>& track) final;
    void onLocalVideoTrackAdded(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& track) final;
    void onLocalAudioTrackAddFailure(std::string id, std::string_view details) final;
    void onLocalVideoTrackAddFailure(std::string id, std::string_view details) final;
    void onLocalAudioTrackRemoved(std::string id) final;
    void onLocalVideoTrackRemoved(std::string id) final;
    void onError(LiveKitCpp::LiveKitError error, const std::string& what) final;
    void onSidChanged(const LiveKitCpp::Participant* participant) final;
    void onIdentityChanged(const LiveKitCpp::Participant* participant) final;
    void onNameChanged(const LiveKitCpp::Participant* participant) final;
    void onSpeakerInfoChanged(const LiveKitCpp::Participant* participant,
                              float level, bool active) final;
    void onStateChanged(LiveKitCpp::SessionState) final;
    void onChatMessageReceived(const LiveKitCpp::ChatMessage& message,
                               const std::string& participantIdentity,
                               const std::vector<std::string>&) final;
    void onRemoteParticipantAdded(const std::string& sid) final;
    void onRemoteParticipantRemoved(const std::string& sid) final;
private:
    LocalParticipant* const _localParticipant;
    std::unique_ptr<LiveKitCpp::Session> _impl;
    QHash<QString, RemoteParticipant*> _remoteParticipants;
    LiveKitCpp::EncryptionType _encryption = LiveKitCpp::EncryptionType::None;
    bool _activeCamera = false;
    bool _activeMicrophone = false;
    bool _activeSharing = false;
    QString _prefferedVideoEncoder;
    QString _prefferedAudioEncoder;
};

#endif // Session_H
