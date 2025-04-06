#include "videotrackwrapper.h"
#include <media/VideoTrack.h>
#include <media/VideoFrame.h>

VideoTrackWrapper::VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl,
                                     QObject *parent)
    : TrackWrapper(impl, parent)
    , _impl(impl)
{
}

VideoTrackWrapper::~VideoTrackWrapper()
{
    if (_impl) {
        _impl->removeSink(this);
    }
}

void VideoTrackWrapper::setVideoOutput(QVideoSink* output)
{
    if (_impl && _output.get() != output) {
        _output.set(output);
        if (!output) {
            _impl->removeSink(this);
        }
        else {
            _impl->addSink(this);
        }
    }
}

void VideoTrackWrapper::onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame)
{
    if (frame && frame->planesCount()) {

    }
}
