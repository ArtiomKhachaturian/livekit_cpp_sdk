#ifndef VIDEOSINK_H
#define VIDEOSINK_H
#include "safeobj.h"
#include <livekit/media/CameraEventsListener.h>
#include <livekit/media/VideoSink.h>
#include <QObject>
#include <QQmlEngine>
#include <QBasicTimer>
#include <QPointer>
#include <QReadWriteLock>
#include <QSize>
#include <QVideoSink>
#include <atomic>

class VideoSink : public QObject,
                  protected LiveKitCpp::VideoSink,
                  protected LiveKitCpp::CameraEventsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoSink)
    QML_UNCREATABLE("Create CameraDevice or CameraTrack instead of")
    Q_PROPERTY(QVideoSink* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged FINAL)
    Q_PROPERTY(quint16 fps READ fps NOTIFY fpsChanged FINAL)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
public:
    explicit VideoSink(QObject *parent = nullptr);
    ~VideoSink() override;
    Q_INVOKABLE QVideoSink* videoOutput() const;
    Q_INVOKABLE quint16 fps() const noexcept { return _fps; }
    Q_INVOKABLE QSize frameSize() const { return _frameSize; }
    Q_INVOKABLE bool isActive() const { return _active; }
public slots:
    void setVideoOutput(QVideoSink* output);
signals:
    void videoOutputChanged();
    void fpsChanged();
    void frameSizeChanged();
    void activeChanged();
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
    void setActive(bool active);
    void setFps(quint16 fps);
    void setFrameSize(QSize frameSize, bool updateFps = true);
    void setFrameSize(int width, int height, bool updateFps = true);
    // impl. of LiveKitCpp::VideoSink
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) override;
    // impl. of LiveKitCpp::CameraEventsListener
    void onCapturingStarted(const std::string&, const LiveKitCpp::CameraOptions&) override;
    void onCapturingStartFailed(const std::string&, const LiveKitCpp::CameraOptions&) override;
    void onCapturingStopped(const std::string&) override { setActive(false); }
    void onCapturingFatalError(const std::string&) override { setActive(false); }
private:
    static constexpr QSize _nullSize = {0, 0};
    QBasicTimer _fpsTimer;
    mutable QReadWriteLock _outputLock;
    QPointer<QVideoSink> _output;
    quint16 _fps = 0U;
    std::atomic<quint16> _framesCounter = 0U;
    SafeObj<QSize> _frameSize;
    std::atomic_bool _active = false;
};

Q_DECLARE_METATYPE(VideoSink*)

#endif // VIDEOSINK_H
