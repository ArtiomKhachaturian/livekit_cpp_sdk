#ifndef AUDIOTRACKWRAPPER_H
#define AUDIOTRACKWRAPPER_H
#include <media/MediaEventsListener.h>
#include <QObject>
#include <QMetaType>
#include <QtQml/qqmlregistration.h>

namespace LiveKitCpp {
class AudioTrack;
}

class AudioTrackWrapper : public QObject, private LiveKitCpp::MediaEventsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AudioTrackWrapper)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
public:
    AudioTrackWrapper(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl = {},
                      QObject *parent = nullptr);
    ~AudioTrackWrapper();
    std::shared_ptr<LiveKitCpp::AudioTrack> track() const noexcept { return _impl.lock(); }
    Q_INVOKABLE QString id() const;
    Q_INVOKABLE bool muted() const;
public slots:
    void setVolume(qreal volume);
    void setMuted(bool muted);
signals:
    void muteChanged();
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool) final { emit muteChanged(); }
private:
    const std::weak_ptr<LiveKitCpp::AudioTrack> _impl;
};

Q_DECLARE_METATYPE(AudioTrackWrapper*)

#endif // AUDIOTRACKWRAPPER_H
