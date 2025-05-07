#include "videosource.h"
#include "videofilter.h"
#include <QtCore/qthread.h>
#include <livekit/rtc/media/qt/VideoFrameQtHelper.h>

namespace
{

inline QString toString(const QSize& s) {
    if (!s.isEmpty()) {
        return QString::number(s.width()) + QStringLiteral("x") + QString::number(s.height());
    }
    return {};
}

}


VideoSource::VideoSource(QObject *parent)
    : QObject{parent}
    , _frameType(LiveKitCpp::VideoFrameType::I420)
{
    QObject::connect(&_fpsMeter, &FpsMeter::fpsChanged, this, &VideoSource::onFpsChanged);
}

VideoSource::~VideoSource()
{
    _fpsMeter.stop();
    const QWriteLocker locker(&_outputs._lock);
    for (qsizetype i = 0; i < _outputs->size(); ++i) {
        if (const auto& sink = _outputs->at(i)) {
            sink->disconnect(this);
        }
    }
    _outputs->clear();
}

QString VideoSource::frameType() const
{
    return QString::fromStdString(LiveKitCpp::toString(_frameType));
}

QString VideoSource::filter() const
{
    if (_filter) {
        return _filter->name();
    }
    return {};
}

QString VideoSource::stats() const
{
    return formatVideoInfo(frameSize(), _fpsMeter.fps());
}

void VideoSource::addOutput(QVideoSink* output)
{
    if (output) {
        bool activate = false;
        {
            const QWriteLocker locker(&_outputs._lock);
            if (-1 == _outputs->indexOf(output)) {
                QObject::connect(output, &QVideoSink::destroyed, this, &VideoSource::removeSink);
                _outputs->append(output);
                // 1st sink in collection
                activate = 1 == _outputs->size();
            }
        }
        if (activate) {
            subsribe(true);
            setActive(true);
        }
    }
}

void VideoSource::removeOutput(QVideoSink* output)
{
    removeSink(output);
}

void VideoSource::setFilter(const QString& filter)
{
    if (filter != this->filter()) {
        if (filter.isEmpty()) {
            applyFilter(nullptr);
            _filter.reset();
            emit filterChanged();
        }
        else if (const auto newFilter = VideoFilter::create(filter)) {
            applyFilter(newFilter);
            _filter.reset(newFilter);
            emit filterChanged();
        }
    }
}

QString VideoSource::formatVideoInfo(const QSize& frameSize, quint16 fps) const
{
    const auto frameInfo = toString(frameSize);
    if (!frameInfo.isEmpty()) {
        return frameType() + QStringLiteral(": ") + frameInfo + tr(" %1 fps").arg(fps);
    }
    return {};
}

QString VideoSource::formatVideoInfo(int frameWidth, int frameHeight, quint16 fps) const
{
    return formatVideoInfo(QSize(frameWidth, frameHeight), fps);
}

void VideoSource::startMetricsCollection()
{
    if (isActive() && metricsAllowed()) {
         _fpsMeter.start();
    }
}

void VideoSource::stopMetricsCollection()
{
    _fpsMeter.stop();
    setFrameSize(_nullSize);
}

void VideoSource::removeSink(QObject* sink)
{
    if (sink) {
        bool deactivate = false, removed = false;
        {
            const QWriteLocker locker(&_outputs._lock);
            for (qsizetype i = 0; i < _outputs->size(); ++i) {
                if (_outputs->at(i) == sink) {
                    _outputs->removeAt(i);
                    removed = true;
                }
            }
            if (removed) {
                deactivate = _outputs->isEmpty();
            }
        }
        if (removed) {
            sink->disconnect(this);
        }
        if (deactivate) {
            setActive(false);
            subsribe(false);
        }
    }
}

void VideoSource::sendFrame(const QVideoFrame& frame)
{
    if (frame.isValid()) {
        const QReadLocker locker(&_outputs._lock);
        for (qsizetype i = 0; i < _outputs->size(); ++i) {
            if (const auto& sink = _outputs->at(i)) {
                sink->setVideoFrame(frame);
            }
        }
    }
}

void VideoSource::setFrameType(LiveKitCpp::VideoFrameType type)
{
    if (type != _frameType.exchange(type)) {
        if (metricsAllowed()) {
            emit statsChanged();
        }
    }
}

void VideoSource::setActive(bool active)
{
    if (active != _active.exchange(active)) {
        if (active) {
            startMetricsCollection();
        }
        else {
            stopMetricsCollection();
        }
        emit activeChanged();
    }
}

void VideoSource::setFrameSize(QSize frameSize)
{
    if (_frameSize.exchange(std::move(frameSize))) {
        if (metricsAllowed()) {
            emit statsChanged();
        }
    }
}

void VideoSource::setFrameSize(int width, int height)
{
    setFrameSize(QSize(width, height));
}

void VideoSource::onFpsChanged()
{
    if (metricsAllowed()) {
        emit statsChanged();
    }
}

void VideoSource::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && isActive()) {
        const auto qtFrame = LiveKitCpp::convert(frame);
        if (qtFrame.isValid()) {
            sendFrame(qtFrame);
            if (!isMuted()) {
                _fpsMeter.addFrame();
                setFrameType(frame->type());
                setFrameSize(qtFrame.width(), qtFrame.height());
            }
            else {
                setFrameSize(_nullSize);
            }
        }
    }
}

