#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H
#include "lockable.h"
#include <livekit/rtc/media/LocalVideoFilterPin.h>
#include <QVideoFrame>
#include <atomic>

class VideoFilter : public LiveKitCpp::LocalVideoFilterPin
{
    class RGB24Frame;
public:
    void setPaused(bool paused) { _paused = paused; }
    // impl. of LiveKitCpp::LocalVideoFilterPin
    bool paused() const final { return _paused; }
    void setReceiver(LiveKitCpp::VideoSink* sink) final;
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
protected:
    VideoFilter() = default;
    virtual void processFrame(QVideoFrame frame);
    virtual void processFrame(QImage&& image) = 0;
    void sendProcessed(QImage&& image, int64_t timestampUs = 0LL);
    void sendProcessed(const QVideoFrame& frame);
    bool hasReceiver() const;
private:
    Lockable<LiveKitCpp::VideoSink*> _receiver = nullptr;
    std::atomic_bool _paused = false;
};

#endif // VIDEOFILTER_H
