#include "videofilter.h"
#include <livekit/rtc/media/VideoFrameQtHelper.h>

class VideoFilter::RGB24Frame : public LiveKitCpp::VideoFrame
{
public:
    RGB24Frame(QImage image, int64_t timestampUs = 0LL);
    int width() const final { return _image.width(); }
    int height() const final { return _image.height(); }
    int stride(size_t) const final { return _image.bytesPerLine(); }
    const std::byte* data(size_t) const final { return reinterpret_cast<const std::byte*>(_image.bits()); }
    int dataSize(size_t) const final { return _image.sizeInBytes(); }
private:
    const QImage _image;
};

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
        image = image.convertToFormat(QImage::Format_RGB888);
        const QReadLocker locker(&_receiver._lock);
        if (const auto receiver = _receiver._val) {
            receiver->onFrame(std::make_shared<RGB24Frame>(std::move(image), timestampUs));
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

VideoFilter::RGB24Frame::RGB24Frame(QImage image, int64_t timestampUs)
    : LiveKitCpp::VideoFrame(LiveKitCpp::VideoFrameType::RGB24, 0, timestampUs)
    , _image(std::move(image))
{
}
