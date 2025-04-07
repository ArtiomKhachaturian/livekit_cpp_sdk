#include "videotrackwrapper.h"
#include <media/VideoTrack.h>
#include <media/VideoFrame.h>
#include <media/VideoFrameQtHelper.h>

#include <QDebug>

VideoTrackWrapper::VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl,
                                     QObject *parent)
    : TrackWrapper(impl, parent)
    , _impl(impl)
{
    _fpsTimer.setInterval(1000);
    _fpsTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&_fpsTimer, &QTimer::timeout, this, &VideoTrackWrapper::onMeasureFramerate);
}

VideoTrackWrapper::~VideoTrackWrapper()
{
    if (const auto impl = _impl.lock()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            impl->removeSink(this);
        }
    }
    _fpsTimer.disconnect(this);
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
            _fpsTimer.start();
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

void VideoTrackWrapper::onMeasureFramerate()
{
    const auto fps = _framesCounter.exchange(0U);
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
    }
}

void VideoTrackWrapper::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output->setVideoFrame(qtFrame);
                _framesCounter.fetch_add(1U);
            }
        }
    }
}
