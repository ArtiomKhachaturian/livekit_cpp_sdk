#include "localvideodevice.h"
#include <livekit/rtc/media/LocalVideoDevice.h>

LocalVideoDevice::LocalVideoDevice(QObject *parent)
    : VideoSource(parent)
{
}

LocalVideoDevice::LocalVideoDevice(std::shared_ptr<LiveKitCpp::LocalVideoDevice> device,
                                   QObject *parent)
    : VideoSource(parent)
    , _device(std::move(device))
{
    if (_device) {
        _device->addListener(this);
    }
}

LocalVideoDevice::~LocalVideoDevice()
{
    LocalVideoDevice::subsribe(false);
    if (_device) {
        _device->removeListener(this);
    }
}

void LocalVideoDevice::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (_device) {
        _device->setDeviceInfo(info);
    }
}

QString LocalVideoDevice::name() const
{
    if (_device) {
        return QString::fromStdString(_device->deviceInfo()._name);
    }
    return VideoSource::name();
}

bool LocalVideoDevice::isMuted() const
{
    return _device && _device->muted();
}

void LocalVideoDevice::subsribe(bool subscribe)
{
    if (subscribe) {
        _device->addSink(this);
    }
    else {
        _device->removeSink(this);
    }
}
