#include "videosinkwrapper.h"
#include <media/VideoFrame.h>
#include <media/VideoFrameQtHelper.h>
#include <QTimerEvent>

using namespace std::chrono_literals;

VideoSinkWrapper::VideoSinkWrapper(QObject *parent)
    : QObject{parent}
{
}

VideoSinkWrapper::~VideoSinkWrapper()
{
    stopMetricsCollection();
}

QVideoSink* VideoSinkWrapper::videoOutput() const
{
    const QReadLocker locker(&_outputLock);
    return _output;
}

void VideoSinkWrapper::setVideoOutput(QVideoSink* output)
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
        if (output) {
            startMetricsCollection();
        }
        else {
            stopMetricsCollection();
        }
        emit videoOutputChanged();
    }
}

void VideoSinkWrapper::startMetricsCollection()
{
    _fpsTimer.start(1000ms, this);
}

void VideoSinkWrapper::stopMetricsCollection()
{
    _fpsTimer.stop();
    _framesCounter = 0U;
    setFps(0U);
    setFrameSize(_nullSize, false);
}

bool VideoSinkWrapper::hasOutput() const
{
    const QReadLocker locker(&_outputLock);
    return nullptr != _output;
}

void VideoSinkWrapper::timerEvent(QTimerEvent* e)
{
    if (e && e->timerId() == _fpsTimer.timerId()) {
        setFps(_framesCounter.exchange(0U));
    }
    QObject::timerEvent(e);
}

void VideoSinkWrapper::setFps(quint16 fps)
{
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
    }
}

void VideoSinkWrapper::setFrameSize(QSize frameSize, bool updateFps)
{
    if (updateFps) {
        _framesCounter.fetch_add(1U);
    }
    if (_frameSize.exchange(std::move(frameSize))) {
        emit frameSizeChanged();
    }
}

void VideoSinkWrapper::setFrameSize(int width, int height, bool updateFps)
{
    setFrameSize(QSize(width, height), updateFps);
}

void VideoSinkWrapper::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output->setVideoFrame(qtFrame);
                if (!isMuted()) {
                    setFrameSize(qtFrame.width(), qtFrame.height());
                }
                else {
                    setFrameSize(_nullSize, false);
                }
            }
        }
    }
}
