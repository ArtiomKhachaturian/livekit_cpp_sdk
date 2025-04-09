#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H
#include <livekit/media/MediaEventsListener.h>
#include <QObject>
#include <QMetaType>
#include <QtQml/qqmlregistration.h>

namespace LiveKitCpp {
class AudioTrack;
}

class AudioTrack : public QObject, private LiveKitCpp::MediaEventsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AudioTrack)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
public:
    explicit AudioTrack(QObject* parent = nullptr);
    AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl,
               QObject* parent = nullptr);
    ~AudioTrack() override;
    const auto& track() const noexcept { return _impl; }
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
    const std::shared_ptr<LiveKitCpp::AudioTrack> _impl;
};

Q_DECLARE_METATYPE(AudioTrack*)

#endif // AUDIOTRACK_H
