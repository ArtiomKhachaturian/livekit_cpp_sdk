#ifndef VIDEOTRACKWRAPPER_H
#define VIDEOTRACKWRAPPER_H
#include "trackwrapper.h"
#include <media/VideoTrackSink.h>
#include <QPointer>
#include <QReadWriteLock>
#include <QVideoSink>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrackWrapper : public TrackWrapper, private LiveKitCpp::VideoTrackSink
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrackWrapper)
    Q_PROPERTY(QVideoSink* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
public:
    VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl = {},
                      QObject *parent = nullptr);
    ~VideoTrackWrapper() override;
    std::shared_ptr<LiveKitCpp::VideoTrack> track() const noexcept { return _impl.lock(); }
    Q_INVOKABLE QVideoSink* videoOutput() const;
public slots:
    void setVideoOutput(QVideoSink* output);
signals:
    void videoOutputChanged();
private:
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
private:
    const std::weak_ptr<LiveKitCpp::VideoTrack> _impl;
    mutable QReadWriteLock _outputLock;
    QPointer<QVideoSink> _output;
};

Q_DECLARE_METATYPE(VideoTrackWrapper*)

#endif // VIDEOTRACKWRAPPER_H
