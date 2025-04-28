#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H
#include "lockable.h"
#include <livekit/rtc/media/LocalVideoFilterPin.h>
#include <QObject>
#include <QVideoFrame>
#include <atomic>

class VideoFilter : public QObject,
                    public LiveKitCpp::LocalVideoFilterPin
{
    Q_OBJECT
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pauseChanged FINAL)
public:
    void setPaused(bool paused);
    const auto& name() const noexcept { return _name; }
    // impl. of LiveKitCpp::LocalVideoFilterPin
    bool paused() const final { return _paused; }
    void setReceiver(LiveKitCpp::VideoSink* sink) final;
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
signals:
    void pauseChanged();
protected:
    VideoFilter(QString name, QObject* parent = nullptr);
    virtual void processFrame(QVideoFrame frame);
    virtual void processFrame(QImage&& image) = 0;
    void sendProcessed(QImage&& image, int64_t timestampUs = 0LL);
    void sendProcessed(const QVideoFrame& frame);
    bool hasReceiver() const;
private:
    const QString _name;
    Lockable<LiveKitCpp::VideoSink*> _receiver = nullptr;
    std::atomic_bool _paused = false;
};

#endif // VIDEOFILTER_H
