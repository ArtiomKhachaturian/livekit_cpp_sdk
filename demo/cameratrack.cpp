#include "cameratrack.h"
#include <livekit/rtc/media/LocalVideoTrack.h>

CameraTrack::CameraTrack(QObject* parent)
    : VideoTrack(parent)
{
}

CameraTrack::CameraTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& impl, QObject* parent)
    : VideoTrack(impl, parent)
    , _impl(impl)
{
}

MediaDeviceInfo CameraTrack::deviceInfo() const
{
    if (const auto impl = _impl.lock()) {
        return impl->deviceInfo();
    }
    return {};
}

VideoOptions CameraTrack::options() const
{
    if (const auto impl = _impl.lock()) {
        return impl->options();
    }
    return {};
}

void CameraTrack::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto impl = _impl.lock()) {
        impl->setDeviceInfo(info);
    }
}

void CameraTrack::setOptions(const VideoOptions& options)
{
    if (const auto impl = _impl.lock()) {
        impl->setOptions(options);
    }
}

void CameraTrack::onMediaChanged(const std::string&)
{
    emit deviceInfoChanged();
}

void CameraTrack::onMediaOptionsChanged(const std::string&)
{
    emit optionsChanged();
}
