#include "videofilter.h"
#include "grayscalevideofilter.h"
#include "blurvideofilter.h"
#include "sepiavideofilter.h"
#include "pencilvideofilter.h"
#include <livekit/rtc/media/qt/VideoFrameQtHelper.h>

VideoFilter::VideoFilter(QString name, QObject* parent)
    : QObject(parent)
    , _name(std::move(name))
{
    Q_ASSERT(!_name.isEmpty());
}

VideoFilter* VideoFilter::create(const QString& name, QObject* parent)
{
    if (!name.isEmpty()) {
        if (const auto filter = createFilter<BlurVideofilter>(name, parent)) {
            return filter;
        }
        if (const auto filter = createFilter<GrayscaleVideoFilter>(name, parent)) {
            return filter;
        }
        if (const auto filter = createFilter<SepiaVideoFilter>(name, parent)) {
            return filter;
        }
        if (const auto filter = createFilter<PencilVideoFilter>(name, parent)) {
            return filter;
        }
    }
    return nullptr;
}

QStringList VideoFilter::available()
{
    return {{BlurVideofilter::filterName(), GrayscaleVideoFilter::filterName(),
             PencilVideoFilter::filterName(), SepiaVideoFilter::filterName()}};
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

template <class TFilter>
VideoFilter* VideoFilter::createFilter(const QString& name, QObject* parent)
{
    if (0 == name.compare(TFilter::filterName(), Qt::CaseInsensitive)) {
        return new TFilter(parent);
    }
    return nullptr;
}
