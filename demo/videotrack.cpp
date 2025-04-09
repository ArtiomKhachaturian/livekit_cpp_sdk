#include "videotrack.h"
#include <livekit/media/VideoTrack.h>

VideoTrack::VideoTrack(QObject* parent)
    : VideoSink{parent}
{
}

VideoTrack::VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl,
                                     QObject* parent)
    : VideoSink(parent)
    , _impl(impl)
{
    if (_impl) {
        _impl->addListener(this);
    }
}

VideoTrack::~VideoTrack()
{
    if (_impl) {
        _impl->removeListener(this);
        _impl->removeSink(this);
    }
}

QString VideoTrack::id() const
{
    if (_impl) {
        return QString::fromStdString(_impl->id());
    }
    return {};
}

bool VideoTrack::muted() const
{
    return _impl && _impl->muted();
}

void VideoTrack::setMuted(bool mute)
{
    if (_impl) {
        _impl->mute(mute);
    }
}

void VideoTrack::subsribe(bool subscribe)
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

void VideoTrack::onMuteChanged(const std::string&, bool muted)
{
    if (muted) {
        stopMetricsCollection();
    }
    else {
        startMetricsCollection();
    }
    emit muteChanged();
}

