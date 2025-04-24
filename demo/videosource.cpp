#include "videosource.h"
#include <livekit/rtc/media/VideoFrame.h>
#include <livekit/rtc/media/VideoFrameQtHelper.h>
#include <QThread>

VideoSource::VideoSource(QObject *parent)
    : QObject{parent}
    , _frameType(LiveKitCpp::VideoFrameType::I420)
{
    QObject::connect(&_fpsMeter, &FpsMeter::fpsChanged, this, &VideoSource::setFps);
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
        if (QThread::currentThread() == thread()) {
            _fpsMeter.start();
        }
        else {
            QMetaObject::invokeMethod(this, &VideoSource::startMetricsCollection);
        }
    }
}

void VideoSource::stopMetricsCollection()
{
    _fpsMeter.stop();
    setFps(0U);
    setFrameSize(_nullSize, false);
    if (QThread::currentThread() == thread()) {
        _fpsMeter.stop();
    }
    else {
        QMetaObject::invokeMethod(this, &VideoSource::stopMetricsCollection);
    }
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
        subsribe(active);
        if (active) {
            startMetricsCollection();
        }
        else {
            stopMetricsCollection();
        }
        emit activeChanged();
    }
}

void VideoSource::setFps(quint16 fps)
{
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
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

