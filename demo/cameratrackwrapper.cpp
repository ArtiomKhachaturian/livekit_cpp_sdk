#include "cameratrackwrapper.h"
#include <media/CameraTrack.h>

CameraTrackWrapper::CameraTrackWrapper(const std::shared_ptr<LiveKitCpp::CameraTrack>& impl,
                                       QObject *parent)
    : VideoSinkWrapper(parent)
    , _impl(impl)
{
    if (impl) {
        impl->addListener(this);
    }
}

CameraTrackWrapper::~CameraTrackWrapper()
{
    CameraTrackWrapper::subsribe(false);
    if (const auto impl = _impl.lock()) {
        impl->removeListener(this);
    }
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

QString CameraTrackWrapper::id() const
{
    if (const auto impl = _impl.lock()) {
        return QString::fromStdString(impl->id());
    }
    return {};
}

bool CameraTrackWrapper::muted() const
{
    if (const auto impl = _impl.lock()) {
        return impl->muted();
    }
    return false;
}

void CameraTrackWrapper::setMuted(bool mute)
{
    const auto impl = _impl.lock();
    if (impl && impl->muted() != mute) {
        impl->mute(mute);
        if (mute) {
            stopMetricsCollection();
        }
        else if (hasOutput()) {
            startMetricsCollection();
        }
    }
}

void CameraTrackWrapper::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto impl = _impl.lock()) {
        impl->setDeviceInfo(info);
    }
}

void CameraTrackWrapper::setOptions(const CameraOptions& options)
{
    if (const auto impl = _impl.lock()) {
        impl->setOptions(options);
    }
}

void CameraTrackWrapper::subsribe(bool subscribe)
{
    if (const auto impl = _impl.lock()) {
        if (subscribe) {
            impl->addSink(this);
        }
        else {
            impl->removeSink(this);
        }
    }
}

void CameraTrackWrapper::onCapturerChanged(const std::string&, const LiveKitCpp::MediaDeviceInfo&)
{
    emit deviceInfoChanged();
}

void CameraTrackWrapper::onOptionsChanged(const std::string&, const LiveKitCpp::CameraOptions&)
{
    emit optionsChanged();
}
