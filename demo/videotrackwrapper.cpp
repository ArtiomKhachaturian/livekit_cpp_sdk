#include "videotrackwrapper.h"
#include <media/VideoTrack.h>
#include <media/VideoFrame.h>
#include <media/VideoFrameQtHelper.h>

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
        }
    }
    if (changed) {
        emit videoOutputChanged();
    }
}

QVideoSink* VideoTrackWrapper::videoOutput() const
{
    const QReadLocker locker(&_outputLock);
    return _output;
}

void VideoTrackWrapper::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {
        const QReadLocker locker(&_outputLock);
        if (_output) {
            const auto qtFrame = LiveKitCpp::convert(frame);
            if (qtFrame.isValid()) {
                _output->setVideoFrame(qtFrame);
            }
        }
    }
}
