#include "videotrack.h"
#include <livekit/media/VideoTrack.h>

VideoTrack::VideoTrack(QObject* parent)
    : VideoSource{parent}
{
}

VideoTrack::VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack,
                       QObject* parent)
    : VideoSource(parent)
    , _sdkTrack(sdkTrack)
{
    if (_sdkTrack) {
        _sdkTrack->addListener(this);
    }
}

VideoTrack::~VideoTrack()
{
    takeSdkTrack();
}

std::shared_ptr<LiveKitCpp::VideoTrack> VideoTrack::takeSdkTrack()
{
    if (_sdkTrack) {
        _sdkTrack->removeListener(this);
        _sdkTrack->removeSink(this);
        return std::move(_sdkTrack);
    }
    return {};
}

QString VideoTrack::id() const
{
    if (_sdkTrack) {
        return QString::fromStdString(_sdkTrack->id());
    }
    return {};
}

bool VideoTrack::muted() const
{
    if (_sdkTrack) {
        if (_sdkTrack->muted()) {
            return true;
        }
        // ignore notifications from remote participants that
        // my video stream was disabled on their side
        // we accept only of their stream changes
        if (_sdkTrack->remote() && _sdkTrack->remoteMuted()) {
            return true;
        }
    }
    return false;
}

bool VideoTrack::isScreencast() const
{
    return _sdkTrack && LiveKitCpp::TrackSource::ScreenShare == _sdkTrack->source();
}

bool VideoTrack::isRemote() const
{
    return _sdkTrack && _sdkTrack->remote();
}

void VideoTrack::setMuted(bool mute)
{
    if (_sdkTrack) {
        _sdkTrack->mute(mute);
    }
}

void VideoTrack::subsribe(bool subscribe)
{
    if (_sdkTrack) {
        if (subscribe) {
            _sdkTrack->addSink(this);
        }
        else {
            _sdkTrack->removeSink(this);
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

void VideoTrack::onRemoteSideMuteChanged(const std::string&, bool)
{
    emit muteChanged();
}

