#include "videotrackwrapper.h"
#include <media/VideoTrack.h>
#include <media/VideoFrame.h>
#include <media/VideoFrameQtHelper.h>
#include <QTimerEvent>

using namespace std::chrono_literals;

VideoTrackWrapper::VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl,
                                     QObject *parent)
    : TrackWrapper(impl, parent)
    , _impl(impl)
{
}

VideoTrackWrapper::~VideoTrackWrapper()
{
    if (const auto impl = _impl.lock()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            impl->removeSink(this);
        }
    }
    _fpsTimer.stop();
}

void VideoTrackWrapper::setVideoOutput(QVideoSink* output)
{
    bool changed = false;
    if (const auto impl = _impl.lock()) {
        const QWriteLocker locker(&_outputLock);
        if (output != _output) {
            if (output && !_output) {
                impl->addSink(this);
            }
            else if (!output && _output) {
                impl->removeSink(this);
            }
            _output = output;
            changed = true;
            _framesCounter = 0U;
        }
    }
    if (changed) {
        if (output) {
            _fpsTimer.start(1000ms, this);
        }
        else {
            _fpsTimer.stop();
        }
        emit videoOutputChanged();
    }
}

QVideoSink* VideoTrackWrapper::videoOutput() const
{
    const QReadLocker locker(&_outputLock);
    return _output;
}

void VideoTrackWrapper::timerEvent(QTimerEvent* e)
{
    if (e && e->timerId() == _fpsTimer.timerId()) {
        setFps(_framesCounter.exchange(0U));
    }
    TrackWrapper::timerEvent(e);
}

void VideoTrackWrapper::setFps(quint16 fps)
{
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
    }
}

void VideoTrackWrapper::setFrameSize(QSize frameSize)
{
    if (_frameSize.exchange(std::move(frameSize))) {
        emit frameSizeChanged();
    }
    _framesCounter.fetch_add(1U);
}

void VideoTrackWrapper::setFrameSize(int width, int height)
{
    setFrameSize(QSize(width, height));
}

void VideoTrackWrapper::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output->setVideoFrame(qtFrame);
                setFrameSize(qtFrame.width(), qtFrame.height());
            }
        }
    }
}
