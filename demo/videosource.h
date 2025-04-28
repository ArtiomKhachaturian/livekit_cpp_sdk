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
    QML_UNCREATABLE("Create LocalVideoDevice or LocalVideoTrack instead of")
    Q_PROPERTY(quint16 fps READ fps NOTIFY fpsChanged FINAL)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged FINAL)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(QString frameType READ frameType NOTIFY frameTypeChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged FINAL)
public:
    explicit VideoSource(QObject *parent = nullptr);
    ~VideoSource() override;
    quint16 fps() const noexcept { return _fpsMeter.fps(); }
    QSize frameSize() const { return _frameSize; }
    bool isActive() const { return _active; }
    QString frameType() const;
    QString filter() const;
    virtual QString name() const { return {}; }
    static QStringList availableFilters();
public slots:
    Q_INVOKABLE void addOutput(QVideoSink* output);
    Q_INVOKABLE void removeOutput(QVideoSink* output);
    bool setFilter(const QString& filter);
signals:
    void fpsChanged();
    void frameSizeChanged();
    void activeChanged();
    void frameTypeChanged();
    void nameChanged();
    void filterChanged();
protected:
    void startMetricsCollection();
    void stopMetricsCollection();
    bool isMetricsCollectionStarted() const { return _fpsMeter.isActive(); }
    bool hasOutputs() const;
    virtual bool hasVideoInput() const { return true; }
    virtual bool isMuted() const { return false; }
    virtual void subsribe(bool /*subscribe*/) {}
    virtual void applyFilter(VideoFilter* /*filter*/ = nullptr) {}
private slots:
    void removeSink(QObject* sink);
private:
    void setFrameType(LiveKitCpp::VideoFrameType type);
    void setActive(bool active = true);
    void setInactive() { setActive(false); }
    void setFrameSize(QSize frameSize, bool updateFps = true);
    void setFrameSize(int width, int height, bool updateFps = true);
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
