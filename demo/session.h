#ifndef Session_H
#define Session_H
#include "audiotrack.h"
#include "cameratrack.h"
#include "audiodevice.h"
#include "cameradevice.h"
#include <livekit/SessionListener.h>
#include <livekit/RemoteParticipantListener.h>
#include <livekit/Session.h>
#include <QObject>
#include <QQmlEngine>
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
    Q_PROPERTY(bool connecting READ connecting NOTIFY stateChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString sid READ sid NOTIFY localDataChanged)
    Q_PROPERTY(QString identity READ identity NOTIFY localDataChanged)
    Q_PROPERTY(QString name READ name NOTIFY localDataChanged)
public:
    Session(std::unique_ptr<LiveKitCpp::Session> impl = {}, QObject *parent = nullptr);
    ~Session() override;
    Q_INVOKABLE bool connectToSfu(const QString& url, const QString& token);
    Q_INVOKABLE AudioTrack* addAudioTrack(AudioDevice* device);
    Q_INVOKABLE CameraTrack* addCameraTrack(CameraDevice* device);
    Q_INVOKABLE AudioTrack* addMicrophoneTrack();
    Q_INVOKABLE CameraTrack* addCameraTrack(const MediaDeviceInfo& info = {},
                                            const CameraOptions& options = {});
    Q_INVOKABLE void destroyAudioTrack(AudioTrack* track);
    Q_INVOKABLE void destroyVideoTrack(VideoTrack* track);
    bool connecting() const;
    State state() const;
    QString sid() const;
    QString identity() const;
    QString name() const;
public slots:
    Q_INVOKABLE void disconnectFromSfu();
    Q_INVOKABLE bool sendChatMessage(const QString& message);
signals:
    void chatMessageReceived(const QString& participantIdentity,
                             const QString& message, bool deleted);
    void error(const QString& desc, const QString& details = {});
    void localDataChanged();
    void stateChanged();
private:
    // impl. of SessionListener
    void onError(LiveKitCpp::LiveKitError error, const std::string& what) final;
    void onChanged(const LiveKitCpp::Participant*) final;
    void onStateChanged(LiveKitCpp::SessionState) final;
    void onChatMessageReceived(const std::string& identity,
                               const std::string& message, const std::string&,
                               int64_t, bool deleted, bool) final;
private:
    const std::unique_ptr<LiveKitCpp::Session> _impl;
};

Q_DECLARE_METATYPE(Session*)

#endif // Session_H
