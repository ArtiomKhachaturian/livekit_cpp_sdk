#include "videosource.h"
#include "cameraoptions.h"
#include <livekit/rtc/media/VideoFrame.h>
#include <livekit/rtc/media/VideoFrameQtHelper.h>
#include <QTimerEvent>
#include <QThread>

using namespace std::chrono_literals;

VideoSource::VideoSource(QObject *parent)
    : QObject{parent}
    , _frameType(LiveKitCpp::VideoFrameType::I420)
{
}

VideoSource::~VideoSource()
{
    stopMetricsCollection();
}

QString VideoSource::frameType() const
{
    return CameraOptions::toString(_frameType);
}

void VideoSource::addOutput(QVideoSink* output)
{
    if (output) {
        const QWriteLocker locker(&_outputs._lock);
        if (-1 == _outputs->indexOf(output)) {
            _outputs->append(output);
            if (1 == _outputs->size()) {
                subsribe(true);
                setActive(true);
            }
        }
    }
}

void VideoSource::removeOutput(QVideoSink* output)
{
    if (output) {
        const QWriteLocker locker(&_outputs._lock);
        if (_outputs->removeAll(output) > 0 && _outputs->isEmpty()) {
            subsribe(false);
            setActive(false);
        }
    }
}

void VideoSource::startMetricsCollection()
{
    if (isActive() && hasVideoInput()) {
        if (QThread::currentThread() == thread()) {
            _fpsTimer.start(1000ms, this);
        }
        else {
            QMetaObject::invokeMethod(this, &VideoSource::startMetricsCollection);
        }
    }
}

void VideoSource::stopMetricsCollection()
{
    _framesCounter = 0U;
    setFps(0U);
    setFrameSize(_nullSize, false);
    if (QThread::currentThread() == thread()) {
        _fpsTimer.stop();
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

void VideoSource::timerEvent(QTimerEvent* e)
{
    if (e && e->timerId() == _fpsTimer.timerId()) {
        setFps(_framesCounter.exchange(0U));
    }
    QObject::timerEvent(e);
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
        _framesCounter.fetch_add(1U);
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
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputs._lock);
        if (!_outputs->isEmpty()) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                for (qsizetype i = 0; i < _outputs->size(); ++i) {
                    _outputs->at(i)->setVideoFrame(qtFrame);
                }
                if (isActive() && !isMuted()) {
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

