#include "cameratrackwrapper.h"
#include <media/CameraTrack.h>

CameraTrackWrapper::CameraTrackWrapper(const std::shared_ptr<LiveKitCpp::CameraTrack>& impl,
                                       QObject *parent)
    : VideoTrackWrapper(impl, parent)
    , _impl(impl)
{
}

void CameraTrackWrapper::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (_impl) {
        _impl->setDeviceInfo(info);
        emit deviceInfoChanged();
    }
}

MediaDeviceInfo CameraTrackWrapper::deviceInfo() const
{
    if (_impl) {
        return _impl->deviceInfo();
    }
    return {};
}
