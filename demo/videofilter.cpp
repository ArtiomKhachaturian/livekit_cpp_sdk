#include "videofilter.h"
#include "grayscalevideofilter.h"
#include "blurvideofilter.h"
#include <livekit/rtc/media/qt/VideoFrameQtHelper.h>

VideoFilter::VideoFilter(QString name, QObject* parent)
    : QObject(parent)
    , _name(std::move(name))
{
    Q_ASSERT(!_name.isEmpty());
}

VideoFilter* VideoFilter::create(const QString& name, QObject* parent)
{
    VideoFilter* newFilter = nullptr;
    if (!name.isEmpty()) {
        if (0 == name.compare(BlurVideofilter::blurFilterName(), Qt::CaseInsensitive)) {
            return new BlurVideofilter(parent);
        }
        if (0 == name.compare(GrayscaleVideoFilter::grayscaleFilterName(), Qt::CaseInsensitive)) {
            return new GrayscaleVideoFilter(parent);
        }
    }
    return nullptr;
}

QStringList VideoFilter::available()
{
    return {{BlurVideofilter::blurFilterName(), GrayscaleVideoFilter::grayscaleFilterName()}};
}

void VideoFilter::setPaused(bool paused)
{
    if (paused != _paused.exchange(paused)) {
        emit pauseChanged();
    }
}

void VideoFilter::setReceiver(LiveKitCpp::VideoSink* sink)
{
    const QWriteLocker locker(&_receiver._lock);
    _receiver._val = sink;
}

void VideoFilter::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame) {
        auto videoFrame = LiveKitCpp::convert(frame);
        if (videoFrame.isValid()) {
            processFrame(std::move(videoFrame));
        }
    }
}

void VideoFilter::processFrame(QVideoFrame frame)
{
    processFrame(frame.toImage());
}

void VideoFilter::sendProcessed(QImage&& image, int64_t timestampUs)
{
    if (!image.isNull()) {
        const QReadLocker locker(&_receiver._lock);
        if (const auto receiver = _receiver._val) {
            auto frame = LiveKitCpp::QImageVideoFrame::create(std::move(image), 0, timestampUs);
            if (frame) {
                receiver->onFrame(frame);
            }
        }
    }
}

void VideoFilter::sendProcessed(const QVideoFrame& frame)
{
    if (frame.isValid() && hasReceiver()) {
        sendProcessed(frame.toImage(), frame.startTime());
    }
}

bool VideoFilter::hasReceiver() const
{
    const QReadLocker locker(&_receiver._lock);
    return nullptr != _receiver._val;
}
