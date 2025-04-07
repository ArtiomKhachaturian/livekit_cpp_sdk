#include "cameratrackwrapper.h"
#include <media/CameraTrack.h>

CameraTrackWrapper::CameraTrackWrapper(const std::shared_ptr<LiveKitCpp::CameraTrack>& impl,
                                       QObject *parent)
    : VideoTrackWrapper(impl, parent)
    , _impl(impl)
{
}

MediaDeviceInfo CameraTrackWrapper::deviceInfo() const
{
    if (const auto impl = _impl.lock()) {
        return impl->deviceInfo();
    }
    return {};
}


CameraOptions CameraTrackWrapper::options() const
{
    if (const auto impl = _impl.lock()) {
        return impl->options();
    }
    return {};
}

void CameraTrackWrapper::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto impl = _impl.lock()) {
        impl->setDeviceInfo(info);
        emit deviceInfoChanged();
    }
}

void CameraTrackWrapper::setOptions(const CameraOptions& options)
{
    if (const auto impl = _impl.lock()) {
        impl->setOptions(options);
        emit optionsChanged();
    }
}
