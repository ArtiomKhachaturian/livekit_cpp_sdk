#ifndef VIDEOTRACKWRAPPER_H
#define VIDEOTRACKWRAPPER_H
#include <QObject>
#include <QQmlEngine>
#include "videosinkwrapper.h"
#include <memory>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrackWrapper : public VideoSinkWrapper
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrackWrapper)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
public:
    explicit VideoTrackWrapper(QObject* parent = nullptr);
    VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl, QObject* parent = nullptr);
    ~VideoTrackWrapper() override;
    const auto& track() const noexcept { return _impl; }
    Q_INVOKABLE QString id() const;
    Q_INVOKABLE bool muted() const;
public slots:
    void setMuted(bool muted);
signals:
    void muteChanged();
protected:
    // overrides of VideoSinkWrapper
    bool hasVideoInput() const final { return nullptr != _impl; }
    bool isMuted() const final { return muted(); }
    void subsribe(bool subscribe) final;
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool muted) final;
private:
    const std::shared_ptr<LiveKitCpp::VideoTrack> _impl;
};

Q_DECLARE_METATYPE(VideoTrackWrapper*)

#endif // VIDEOTRACKWRAPPER_H
