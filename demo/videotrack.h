#ifndef VIDEOTRACK_H
#define VIDEOTRACK_H
#include "safeobj.h"
#include "videosource.h"
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrack : public VideoSource
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrack)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
public:
    explicit VideoTrack(QObject* parent = nullptr);
    VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack,
               QObject* parent = nullptr);
    ~VideoTrack() override;
    std::shared_ptr<LiveKitCpp::VideoTrack> takeSdkTrack();
    QString id() const;
    bool muted() const;
public slots:
    void setMuted(bool muted);
signals:
    void muteChanged();
protected:
    // overrides of VideoSource
    bool hasVideoInput() const final { return nullptr != _sdkTrack.get(); }
    bool isMuted() const final { return muted(); }
    void subsribe(bool subscribe) final;
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool muted) final;
private:
    SafeObj<std::shared_ptr<LiveKitCpp::VideoTrack>> _sdkTrack;
};

Q_DECLARE_METATYPE(VideoTrack*)

#endif // VIDEOTRACK_H
