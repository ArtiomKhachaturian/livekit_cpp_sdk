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
    if (sdkTrack) {
        sdkTrack->addListener(this);
    }
}

VideoTrack::~VideoTrack()
{
    takeSdkTrack();
}

std::shared_ptr<LiveKitCpp::VideoTrack> VideoTrack::takeSdkTrack()
{
    if (auto sdkTrack = _sdkTrack.take()) {
        sdkTrack->removeListener(this);
        sdkTrack->removeSink(this);
        return sdkTrack;
    }
    return {};
}

QString VideoTrack::id() const
{
    if (const auto sdkTrack = _sdkTrack.get()) {
        return QString::fromStdString(sdkTrack->id());
    }
    return {};
}

bool VideoTrack::muted() const
{
    const auto sdkTrack = _sdkTrack.get();
    return sdkTrack && sdkTrack->muted();
}

void VideoTrack::setMuted(bool mute)
{
    if (const auto sdkTrack = _sdkTrack.get()) {
        sdkTrack->mute(mute);
    }
}

void VideoTrack::subsribe(bool subscribe)
{
    if (const auto sdkTrack = _sdkTrack.get()) {
        if (subscribe) {
            sdkTrack->addSink(this);
        }
        else {
            sdkTrack->removeSink(this);
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

