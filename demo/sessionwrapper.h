#ifndef SESSIONWRAPPER_H
#define SESSIONWRAPPER_H
#include <SessionListener.h>
#include <Session.h>
#include <QObject>
#include <QQmlEngine>
#include <memory>

class SessionWrapper : public QObject, private LiveKitCpp::SessionListener
{
    Q_OBJECT
    QML_ELEMENT
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
    explicit SessionWrapper(std::unique_ptr<LiveKitCpp::Session> impl = {},
                            QObject *parent = nullptr);
    ~SessionWrapper() override;
    Q_INVOKABLE bool connectToSfu(const QString& url, const QString& token);
    bool connecting() const;
    State state() const;
    QString sid() const;
    QString identity() const;
    QString name() const;
public slots:
    Q_INVOKABLE void disconnectFromSfu();
    Q_INVOKABLE bool sendChatMessage(const QString& message);
    Q_INVOKABLE bool addMicrophoneTrack();
    Q_INVOKABLE bool addCameraTrack();
    Q_INVOKABLE void removeMicrophoneTrack();
    Q_INVOKABLE void removeCameraTrack();
    Q_INVOKABLE void muteMicrophoneTrack(bool mute);
    Q_INVOKABLE void muteCameraTrack(bool mute);
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
    std::shared_ptr<LiveKitCpp::AudioTrack> _micTrack;
    std::shared_ptr<LiveKitCpp::CameraTrack> _cameraTrack;
};

Q_DECLARE_METATYPE(SessionWrapper*)

#endif // SESSIONWRAPPER_H
