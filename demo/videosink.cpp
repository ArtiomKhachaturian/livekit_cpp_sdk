#include "videosink.h"
#include <livekit/media/VideoFrame.h>
#include <livekit/media/VideoFrameQtHelper.h>
#include <QTimerEvent>
#include <QThread>

using namespace std::chrono_literals;

VideoSink::VideoSink(QObject *parent)
    : QObject{parent}
{
}

VideoSink::~VideoSink()
{
    stopMetricsCollection();
}

QVideoSink* VideoSink::videoOutput() const
{
    const QReadLocker locker(&_outputLock);
    return _output;
}

void VideoSink::setVideoOutput(QVideoSink* output)
{
    bool changed = false;
    if (hasVideoInput()) {
        const QWriteLocker locker(&_outputLock);
        if (output != _output) {
            if (output && !_output) {
                subsribe(true);
            }
            else if (!output && _output) {
                subsribe(false);
            }
            _output = output;
            changed = true;
            _framesCounter = 0U;
        }
    }
    if (changed) {
        setActive(nullptr != output);
        emit videoOutputChanged();
    }
}

void VideoSink::startMetricsCollection()
{
    if (isActive() && hasVideoInput()) {
        if (QThread::currentThread() == thread()) {
            _fpsTimer.start(1000ms, this);
        }
        else {
            QMetaObject::invokeMethod(this, &VideoSink::startMetricsCollection);
        }
    }
}

void VideoSink::stopMetricsCollection()
{
    _framesCounter = 0U;
    setFps(0U);
    setFrameSize(_nullSize, false);
    if (QThread::currentThread() == thread()) {
        _fpsTimer.stop();
    }
    else {
        QMetaObject::invokeMethod(this, &VideoSink::stopMetricsCollection);
    }
}

bool VideoSink::hasOutput() const
{
    const QReadLocker locker(&_outputLock);
    return nullptr != _output;
}

void VideoSink::timerEvent(QTimerEvent* e)
{
    if (e && e->timerId() == _fpsTimer.timerId()) {
        setFps(_framesCounter.exchange(0U));
    }
    QObject::timerEvent(e);
}

void VideoSink::setActive(bool active)
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

void VideoSink::setFps(quint16 fps)
{
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
    }
}

void VideoSink::setFrameSize(QSize frameSize, bool updateFps)
{
    if (updateFps) {
        _framesCounter.fetch_add(1U);
    }
    if (_frameSize.exchange(std::move(frameSize))) {
        emit frameSizeChanged();
    }
}

void VideoSink::setFrameSize(int width, int height, bool updateFps)
{
    setFrameSize(QSize(width, height), updateFps);
}

void VideoSink::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output->setVideoFrame(qtFrame);
                if (isActive() && !isMuted()) {
                    setFrameSize(qtFrame.width(), qtFrame.height());
                }
                else {
                    setFrameSize(_nullSize, false);
                }
            }
        }
    }
}


void VideoSink::onCapturingStarted(const std::string&, const LiveKitCpp::CameraOptions&)
{
    setActive(true);
}

void VideoSink::onCapturingStartFailed(const std::string&, const LiveKitCpp::CameraOptions&)
{
    setActive(false);
}

