#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H
#include "fpsmeter.h"
#include "safeobj.h"
#include <livekit/rtc/media/MediaEventsListener.h>
#include <livekit/rtc/media/VideoSink.h>
#include <QObject>
#include <QScopedPointer>
#include <QQmlEngine>
#include <QSize>
#include <QVideoSink>
#include <atomic>

namespace LiveKitCpp {
enum class VideoFrameType;
}

class VideoFilter;

class VideoSource : public QObject,
                    protected LiveKitCpp::VideoSink,
                    protected LiveKitCpp::MediaEventsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoSource)
    QML_UNCREATABLE("Create LocalVideoDevice or VideoTrack instead of")
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged FINAL)
    Q_PROPERTY(QString stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY muteChanged)
public:
    explicit VideoSource(QObject *parent = nullptr);
    ~VideoSource() override;
    Q_INVOKABLE quint16 fps() const noexcept { return _fpsMeter.fps(); }
    Q_INVOKABLE QSize frameSize() const { return _frameSize; }
    Q_INVOKABLE bool isActive() const { return _active; }
    Q_INVOKABLE QString frameType() const;
    Q_INVOKABLE QString filter() const;
    virtual bool isMuted() const { return false; }
    virtual QString stats() const;
    virtual QString name() const { return {}; }
public slots:
    Q_INVOKABLE void addOutput(QVideoSink* output);
    Q_INVOKABLE void removeOutput(QVideoSink* output);
    void setFilter(const QString& filter);
    virtual void setMuted(bool /*muted*/) {}
signals:
    void muteChanged();
    void activeChanged();
    void nameChanged();
    void filterChanged();
    void statsChanged();
protected:
    static QString formatVideoInfo(const QSize& frameSize, quint16 fps);
    static QString formatVideoInfo(int frameWidth, int frameHeight, quint16 fps);
    void startMetricsCollection();
    void stopMetricsCollection();
    bool isMetricsCollectionStarted() const { return _fpsMeter.isActive(); }
    bool hasOutputs() const;
    virtual bool metricsAllowed() const { return true; }
    virtual void subsribe(bool /*subscribe*/) {}
    virtual void applyFilter(VideoFilter* /*filter*/ = nullptr) {}
private slots:
    void removeSink(QObject* sink);
private:
    void setFrameType(LiveKitCpp::VideoFrameType type);
    void setActive(bool active = true);
    void setInactive() { setActive(false); }
    void setFrameSize(QSize frameSize);
    void setFrameSize(int width, int height);
    void onFpsChanged();
    // impl. of LiveKitCpp::VideoSource
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) override;
    // impl. of LiveKitCpp::CameraEventsListener
    void onMediaStarted(const std::string&) override { setActive(); }
    void onMediaStartFailed(const std::string&, const std::string&) override { setInactive(); }
    void onMediaStopped(const std::string&) override { setInactive(); }
    void onMediaFatalError(const std::string&, const std::string&) override { setInactive(); }
private:
    static constexpr QSize _nullSize = {0, 0};
    QScopedPointer<VideoFilter> _filter;
    Lockable<QList<QVideoSink*>> _outputs;
    SafeObj<QSize> _frameSize;
    FpsMeter _fpsMeter;
    std::atomic_bool _active = false;
    std::atomic<LiveKitCpp::VideoFrameType> _frameType;
};

#endif // VIDEOSOURCE_H
