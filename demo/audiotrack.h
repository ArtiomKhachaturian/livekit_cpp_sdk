#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H
#include <livekit/rtc/media/MediaEventsListener.h>
#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace LiveKitCpp {
class AudioTrack;
}

class AudioTrack : public QObject, private LiveKitCpp::MediaEventsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AudioTrack)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY muteChanged)
public:
    explicit AudioTrack(QObject* parent = nullptr);
    AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack,
               QObject* parent = nullptr);
    ~AudioTrack() override;
    QString id() const;
    bool isMuted() const;
public slots:
    void setVolume(qreal volume);
    void setMuted(bool muted);
signals:
    void muteChanged();
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool) final { emit muteChanged(); }
private:
    const std::shared_ptr<LiveKitCpp::AudioTrack> _sdkTrack;
};

#endif // AUDIOTRACK_H
