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
    Q_PROPERTY(bool remote READ isRemote CONSTANT)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY muteChanged)
    Q_PROPERTY(bool secure READ isSecure CONSTANT)
    Q_PROPERTY(bool firstPacketSentOrReceived READ isFirstPacketSentOrReceived NOTIFY firstFrameSentOrReceived)
public:
    explicit AudioTrack(QObject* parent = nullptr);
    AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack,
               QObject* parent = nullptr);
    ~AudioTrack() override;
    QString id() const;
    bool isRemote() const;
    bool isMuted() const;
    bool isSecure() const;
    bool isFirstPacketSentOrReceived() const;
public slots:
    void setVolume(qreal volume);
    void setMuted(bool muted);
signals:
    void muteChanged();
    void firstFrameSentOrReceived();
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool) final { emit muteChanged(); }
    void onFirstFrameSent(const std::string&) final { emit firstFrameSentOrReceived(); }
    void onFirstFrameReceived(const std::string&) final { emit firstFrameSentOrReceived(); }
private:
    const std::shared_ptr<LiveKitCpp::AudioTrack> _sdkTrack;
};

#endif // AUDIOTRACK_H
