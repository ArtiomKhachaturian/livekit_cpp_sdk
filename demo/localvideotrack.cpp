#include "localvideotrack.h"
#include <livekit/rtc/media/LocalVideoTrack.h>

LocalVideoTrack::LocalVideoTrack(QObject* parent)
    : VideoTrack(parent)
{
}

LocalVideoTrack::LocalVideoTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& impl, QObject* parent)
    : VideoTrack(impl, parent)
    , _impl(impl)
{
}

MediaDeviceInfo LocalVideoTrack::deviceInfo() const
{
    if (const auto impl = _impl.lock()) {
        return impl->deviceInfo();
    }
    return {};
}

VideoOptions LocalVideoTrack::options() const
{
    if (const auto impl = _impl.lock()) {
        return impl->options();
    }
    return {};
}

void LocalVideoTrack::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto impl = _impl.lock()) {
        impl->setDeviceInfo(info);
    }
}

void LocalVideoTrack::setOptions(const VideoOptions& options)
{
    if (const auto impl = _impl.lock()) {
        impl->setOptions(options);
    }
}

void LocalVideoTrack::onMediaChanged(const std::string&)
{
    emit deviceInfoChanged();
}

void LocalVideoTrack::onMediaOptionsChanged(const std::string&)
{
    emit optionsChanged();
}
