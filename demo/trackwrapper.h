#ifndef TRACKWRAPPER_H
#define TRACKWRAPPER_H
#include <QObject>
#include <QMetaType>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace LiveKitCpp {
class Track;
}

class TrackWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
public:
    TrackWrapper(const std::weak_ptr<LiveKitCpp::Track>& impl,
                QObject *parent = nullptr);
public slots:
    void setMuted(bool muted);
public:
    Q_INVOKABLE QString id() const;
    Q_INVOKABLE bool muted() const;
signals:
    void muteChanged();
private:
    const std::weak_ptr<LiveKitCpp::Track> _impl;
};

Q_DECLARE_METATYPE(TrackWrapper*)

#endif // TRACKWRAPPER_H
