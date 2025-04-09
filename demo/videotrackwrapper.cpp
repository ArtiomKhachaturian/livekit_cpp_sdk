#include "videotrackwrapper.h"
#include <media/VideoTrack.h>

VideoTrackWrapper::VideoTrackWrapper(QObject* parent)
    : VideoSinkWrapper{parent}
{
}

VideoTrackWrapper::VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl,
                                     QObject* parent)
    : VideoSinkWrapper(parent)
    , _impl(impl)
{
    if (_impl) {
        _impl->addListener(this);
    }
}

VideoTrackWrapper::~VideoTrackWrapper()
{
    if (_impl) {
        _impl->removeListener(this);
        _impl->removeSink(this);
    }
}

QString VideoTrackWrapper::id() const
{
    if (_impl) {
        return QString::fromStdString(_impl->id());
    }
    return {};
}

bool VideoTrackWrapper::muted() const
{
    return _impl && _impl->muted();
}

void VideoTrackWrapper::setMuted(bool mute)
{
    if (_impl) {
        _impl->mute(mute);
    }
}

void VideoTrackWrapper::subsribe(bool subscribe)
{
    if (_impl) {
        if (subscribe) {
            _impl->addSink(this);
        }
        else {
            _impl->removeSink(this);
        }
    }
}

void VideoTrackWrapper::onMuteChanged(const std::string&, bool muted)
{
    if (muted) {
        stopMetricsCollection();
    }
    else {
        startMetricsCollection();
    }
    emit muteChanged();
}

