#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H
#include "safeobj.h"
#include <livekit/media/CameraEventsListener.h>
#include <livekit/media/VideoSink.h>
#include <QObject>
#include <QQmlEngine>
#include <QBasicTimer>
#include <QPointer>
#include <QSize>
#include <QVideoSink>
#include <atomic>

namespace LiveKitCpp {
enum class VideoFrameType;
}

class VideoSource : public QObject,
                    protected LiveKitCpp::VideoSink,
                    protected LiveKitCpp::CameraEventsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoSource)
    QML_UNCREATABLE("Create CameraDevice or CameraTrack instead of")
    Q_PROPERTY(QVideoSink* output READ output WRITE setOutput NOTIFY outputChanged FINAL)
    Q_PROPERTY(quint16 fps READ fps NOTIFY fpsChanged FINAL)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString frameType READ frameType NOTIFY frameTypeChanged FINAL)
public:
    explicit VideoSource(QObject *parent = nullptr);
    ~VideoSource() override;
    QVideoSink* output() const;
    quint16 fps() const noexcept { return _fps; }
    QSize frameSize() const { return _frameSize; }
    bool isActive() const { return _active; }
    QString frameType() const;
public slots:
    void setOutput(QVideoSink* output);
signals:
    void outputChanged();
    void fpsChanged();
    void frameSizeChanged();
    void activeChanged();
    void frameTypeChanged();
protected:
    void startMetricsCollection();
    void stopMetricsCollection();
    bool isMetricsCollectionStarted() const { return _fpsTimer.isActive(); }
    bool hasOutput() const;
    virtual bool hasVideoInput() const { return true; }
    virtual bool isMuted() const { return false; }
    virtual void subsribe(bool /*subscribe*/) {}
    void timerEvent(QTimerEvent* e) override;
private:
    void setFrameType(LiveKitCpp::VideoFrameType type);
    void setActive(bool active);
    void setFps(quint16 fps);
    void setFrameSize(QSize frameSize, bool updateFps = true);
    void setFrameSize(int width, int height, bool updateFps = true);
    // impl. of LiveKitCpp::VideoSource
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) override;
    // impl. of LiveKitCpp::CameraEventsListener
    void onCapturingStarted(const std::string&, const LiveKitCpp::CameraOptions&) override;
    void onCapturingStartFailed(const std::string&, const LiveKitCpp::CameraOptions&) override;
    void onCapturingStopped(const std::string&) override { setActive(false); }
    void onCapturingFatalError(const std::string&) override { setActive(false); }
private:
    static constexpr QSize _nullSize = {0, 0};
    QBasicTimer _fpsTimer;
    quint16 _fps = 0U;
    Lockable<QPointer<QVideoSink>> _output;
    SafeObj<QSize> _frameSize;
    std::atomic<quint16> _framesCounter = 0U;
    std::atomic_bool _active = false;
    std::atomic<LiveKitCpp::VideoFrameType> _frameType;
};

#endif // VIDEOSOURCE_H
