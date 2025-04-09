#include "videosource.h"
#include "cameraoptions.h"
#include <livekit/media/VideoFrame.h>
#include <livekit/media/VideoFrameQtHelper.h>
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

QVideoSink* VideoSource::output() const
{
    const QReadLocker locker(&_output._lock);
    return _output._val;
}

QString VideoSource::frameType() const
{
    return CameraOptions::toString(_frameType);
}

void VideoSource::setOutput(QVideoSink* output)
{
    bool changed = false;
    if (hasVideoInput()) {
        const QWriteLocker locker(&_output._lock);
        if (output != _output._val) {
            if (output && !_output._val) {
                subsribe(true);
            }
            else if (!output && _output._val) {
                subsribe(false);
            }
            _output._val = output;
            changed = true;
            _framesCounter = 0U;
        }
    }
    if (changed) {
        setActive(nullptr != output);
        emit outputChanged();
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

bool VideoSource::hasOutput() const
{
    const QReadLocker locker(&_output._lock);
    return nullptr != _output._val;
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
        const QReadLocker locker(&_output._lock);
        if (_output._val) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output._val->setVideoFrame(qtFrame);
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


void VideoSource::onCapturingStarted(const std::string&, const LiveKitCpp::CameraOptions&)
{
    setActive(true);
}

void VideoSource::onCapturingStartFailed(const std::string&, const LiveKitCpp::CameraOptions&)
{
    setActive(false);
}

