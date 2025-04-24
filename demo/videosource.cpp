#include "videosource.h"
#include <QtCore/qthread.h>
#include <livekit/rtc/media/VideoFrame.h>
#include <livekit/rtc/media/VideoFrameQtHelper.h>

VideoSource::VideoSource(QObject *parent)
    : QObject{parent}
    , _frameType(LiveKitCpp::VideoFrameType::I420)
{
    QObject::connect(&_fpsMeter, &FpsMeter::fpsChanged, this, &VideoSource::fpsChanged);
}

VideoSource::~VideoSource()
{
    stopMetricsCollection();
    const QWriteLocker locker(&_outputs._lock);
    for (qsizetype i = 0; i < _outputs->size(); ++i) {
        _outputs->at(i)->disconnect(this);
    }
    _outputs->clear();
}

QString VideoSource::frameType() const
{
    return QString::fromStdString(LiveKitCpp::toString(_frameType));
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

void VideoSource::startMetricsCollection()
{
    if (isActive() && hasVideoInput()) {
         _fpsMeter.start();
    }
}

void VideoSource::stopMetricsCollection()
{
    _fpsMeter.stop();
    setFrameSize(_nullSize, false);
}

bool VideoSource::hasOutputs() const
{
    const QReadLocker locker(&_outputs._lock);
    return !_outputs->isEmpty();
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

void VideoSource::setFrameType(LiveKitCpp::VideoFrameType type)
{
    if (type != _frameType.exchange(type)) {
        emit frameTypeChanged();
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

void VideoSource::setFrameSize(QSize frameSize, bool updateFps)
{
    if (updateFps) {
        _fpsMeter.addFrame();
    }
    if (_frameSize.exchange(std::move(frameSize))) {
        emit frameSizeChanged();
    }
}

void VideoSource::setFrameSize(int width, int height, bool updateFps)
{
    setFrameSize(QSize(width, height), updateFps);
}

void VideoSource::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount() && isActive()) {
        const QReadLocker locker(&_outputs._lock);
        if (!_outputs->isEmpty()) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                for (qsizetype i = 0; i < _outputs->size(); ++i) {
                    _outputs->at(i)->setVideoFrame(qtFrame);
                }
                if (!isMuted()) {
                    setFrameType(frame->type());
                    setFrameSize(qtFrame.width(), qtFrame.height());
                }
                else {
                    setFrameSize(_nullSize, false);
                }
            }
        }
    }
}

