#ifndef VIDEOTRACKWRAPPER_H
#define VIDEOTRACKWRAPPER_H
#include "trackwrapper.h"
#include <media/VideoTrackSink.h>
#include <QPointer>
#include <QReadWriteLock>
#include <QVideoSink>
#include <QTimer>
#include <atomic>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrackWrapper : public TrackWrapper, private LiveKitCpp::VideoTrackSink
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrackWrapper)
    Q_PROPERTY(QVideoSink* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
    Q_PROPERTY(quint16 fps READ fps NOTIFY fpsChanged)
public:
    VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl = {},
                      QObject *parent = nullptr);
    ~VideoTrackWrapper() override;
    std::shared_ptr<LiveKitCpp::VideoTrack> track() const noexcept { return _impl.lock(); }
    Q_INVOKABLE QVideoSink* videoOutput() const;
    Q_INVOKABLE quint16 fps() const noexcept { return _fps; }
public slots:
    void setVideoOutput(QVideoSink* output);
signals:
    void videoOutputChanged();
    void fpsChanged();
private slots:
    void onMeasureFramerate();
private:
    // impl. of LiveKitCpp::VideoTrackSink
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
private:
    const std::weak_ptr<LiveKitCpp::VideoTrack> _impl;
    QTimer _fpsTimer;
    mutable QReadWriteLock _outputLock;
    QPointer<QVideoSink> _output;
    quint16 _fps = 0U;
    std::atomic<quint16> _framesCounter = 0U;
};

Q_DECLARE_METATYPE(VideoTrackWrapper*)

#endif // VIDEOTRACKWRAPPER_H
