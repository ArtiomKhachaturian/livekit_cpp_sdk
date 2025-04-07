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
    , _frameSize(_nullSize)
{
    QObject::connect(this, &TrackWrapper::muteChanged, this, &VideoTrackWrapper::onMuteChanged);
}

VideoTrackWrapper::~VideoTrackWrapper()
{
    if (const auto impl = _impl.lock()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            impl->removeSink(this);
        }
    }
    stopMetricsCollection();
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
            startMetricsCollection();
        }
        else {
            stopMetricsCollection();
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

void VideoTrackWrapper::onMuteChanged()
{
    if (muted()) {
        stopMetricsCollection();
    }
    else if (hasOutput()) {
        startMetricsCollection();
    }
}

void VideoTrackWrapper::startMetricsCollection()
{
    _fpsTimer.start(1000ms, this);
}

void VideoTrackWrapper::stopMetricsCollection()
{
    _fpsTimer.stop();
    _framesCounter = 0U;
    setFps(0U);
    setFrameSize(_nullSize, false);
}

void VideoTrackWrapper::setFps(quint16 fps)
{
    if (fps != _fps) {
        _fps = fps;
        emit fpsChanged();
    }
}

void VideoTrackWrapper::setFrameSize(QSize frameSize, bool updateFps)
{
    if (updateFps) {
        _framesCounter.fetch_add(1U);
    }
    if (_frameSize.exchange(std::move(frameSize))) {
        emit frameSizeChanged();
    }
}

void VideoTrackWrapper::setFrameSize(int width, int height, bool updateFps)
{
    setFrameSize(QSize(width, height), updateFps);
}

bool VideoTrackWrapper::hasOutput() const
{
    const QReadLocker locker(&_outputLock);
    return nullptr != _output;
}

void VideoTrackWrapper::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output->setVideoFrame(qtFrame);
                if (!muted()) {
                    setFrameSize(qtFrame.width(), qtFrame.height());
                }
                else {
                    setFrameSize(_nullSize, false);
                }
            }
        }
    }
}
