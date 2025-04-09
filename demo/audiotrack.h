#ifndef AUDIOTRACK_H
#define AUDIOTRACK_H
#include "safeobj.h"
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
    AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack,
               QObject* parent = nullptr);
    ~AudioTrack() override;
    std::shared_ptr<LiveKitCpp::AudioTrack> takeSdkTrack();
    QString id() const;
    bool muted() const;
public slots:
    void setVolume(qreal volume);
    void setMuted(bool muted);
signals:
    void muteChanged();
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool) final { emit muteChanged(); }
private:
    SafeObj<std::shared_ptr<LiveKitCpp::AudioTrack>> _sdkTrack;
};

Q_DECLARE_METATYPE(AudioTrack*)

#endif // AUDIOTRACK_H
