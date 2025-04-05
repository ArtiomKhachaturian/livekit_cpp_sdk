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
    explicit SessionWrapper(std::unique_ptr<LiveKitCpp::Session> impl,
                            const QString& clientId, QObject *parent = nullptr);
    ~SessionWrapper() override;
    Q_INVOKABLE bool connectToSfu(const QString& url, const QString& token);
    Q_INVOKABLE void disconnectFromSfu();
    Q_INVOKABLE QString sid() const;
    Q_INVOKABLE QString identity() const;
    Q_INVOKABLE QString name() const;
signals:
    void error(const QString& clientId, const QString& desc, const QString& details = {});
private:
    // impl. of SessionListener
    void onError(LiveKitCpp::LiveKitError error, const std::string& what) final;
private:
    const std::unique_ptr<LiveKitCpp::Session> _impl;
    const QString _clientId;
};

#endif // SESSIONWRAPPER_H
