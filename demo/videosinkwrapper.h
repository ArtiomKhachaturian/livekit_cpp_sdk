#ifndef VIDEOSINKWRAPPER_H
#define VIDEOSINKWRAPPER_H
#include "safeobj.h"
#include <QObject>
#include <QQmlEngine>
#include <QBasicTimer>
#include <media/VideoSink.h>
#include <QPointer>
#include <QReadWriteLock>
#include <QSize>
#include <QVideoSink>
#include <atomic>

class VideoSinkWrapper : public QObject, protected LiveKitCpp::VideoSink
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoSinkWrapper)
    Q_PROPERTY(QVideoSink* videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged FINAL)
    Q_PROPERTY(quint16 fps READ fps NOTIFY fpsChanged FINAL)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged FINAL)
public:
    explicit VideoSinkWrapper(QObject *parent = nullptr);
    ~VideoSinkWrapper() override;
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
    void startMetricsCollection();
    void stopMetricsCollection();
    bool isMetricsCollectionStarted() const { return _fpsTimer.isActive(); }
    bool hasOutput() const;
    virtual bool hasVideoInput() const { return true; }
    virtual bool isMuted() const { return false; }
    virtual void subsribe(bool /*subscribe*/) {}
    void timerEvent(QTimerEvent* e) override;
private:
    void setFps(quint16 fps);
    void setFrameSize(QSize frameSize, bool updateFps = true);
    void setFrameSize(int width, int height, bool updateFps = true);
    // impl. of LiveKitCpp::VideoSink
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
private:
    static constexpr QSize _nullSize = {0, 0};
    QBasicTimer _fpsTimer;
    mutable QReadWriteLock _outputLock;
    QPointer<QVideoSink> _output;
    quint16 _fps = 0U;
    std::atomic<quint16> _framesCounter = 0U;
    SafeObj<QSize> _frameSize;
};

Q_DECLARE_METATYPE(VideoSinkWrapper*)

#endif // VIDEOSINKWRAPPER_H
