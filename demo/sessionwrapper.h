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
    Q_INVOKABLE void disconnectFromSfu();
    bool connecting() const;
    State state() const;
    QString sid() const;
    QString identity() const;
    QString name() const;
signals:
    void error(const QString& desc, const QString& details = {});
    void localDataChanged();
    void stateChanged();
private:
    // impl. of SessionListener
    void onError(LiveKitCpp::LiveKitError error, const std::string& what) final;
    void onChanged(const LiveKitCpp::Participant*) final;
    void onStateChanged(LiveKitCpp::SessionState) final;
private:
    const std::unique_ptr<LiveKitCpp::Session> _impl;
};

Q_DECLARE_METATYPE(SessionWrapper*)

#endif // SESSIONWRAPPER_H
