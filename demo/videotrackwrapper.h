#ifndef VIDEOTRACKWRAPPER_H
#define VIDEOTRACKWRAPPER_H
#include "trackwrapper.h"
#include "safeobj.h"
#include <QBasicTimer>
#include <media/VideoTrackSink.h>
#include <QPointer>
#include <QReadWriteLock>
#include <QSize>
#include <QVideoSink>
#include <atomic>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrackWrapper : public TrackWrapper, private LiveKitCpp::VideoTrackSink
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrackWrapper)
    Q_PROPERTY(QVideoSink* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged FINAL)
    Q_PROPERTY(quint16 fps READ fps NOTIFY fpsChanged FINAL)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged FINAL)
public:
    VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl = {},
                      QObject *parent = nullptr);
    ~VideoTrackWrapper() override;
    std::shared_ptr<LiveKitCpp::VideoTrack> track() const noexcept { return _impl.lock(); }
    Q_INVOKABLE QVideoSink* videoOutput() const;
    Q_INVOKABLE quint16 fps() const noexcept { return _fps; }
    Q_INVOKABLE QSize frameSize() const { return _frameSize; }
public slots:
    void setVideoOutput(QVideoSink* output);
signals:
    void videoOutputChanged();
    void fpsChanged();
    void frameSizeChanged();
protected:
    void timerEvent(QTimerEvent* e) override;
private slots:
    void onMuteChanged();
private:
    void startMetricsCollection();
    void stopMetricsCollection();
    void setFps(quint16 fps);
    void setFrameSize(QSize frameSize, bool updateFps = true);
    void setFrameSize(int width, int height, bool updateFps = true);
    bool hasOutput() const;
    // impl. of LiveKitCpp::VideoTrackSink
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
private:
    static constexpr QSize _nullSize = {0, 0};
    const std::weak_ptr<LiveKitCpp::VideoTrack> _impl;
    QBasicTimer _fpsTimer;
    mutable QReadWriteLock _outputLock;
    QPointer<QVideoSink> _output;
    quint16 _fps = 0U;
    std::atomic<quint16> _framesCounter = 0U;
    SafeObj<QSize> _frameSize;
};

Q_DECLARE_METATYPE(VideoTrackWrapper*)

#endif // VIDEOTRACKWRAPPER_H
