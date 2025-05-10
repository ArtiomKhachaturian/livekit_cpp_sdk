#include "localvideodevice.h"
#include "videofilter.h"
#include <livekit/rtc/media/LocalVideoDevice.h>

LocalVideoDevice::LocalVideoDevice(QObject *parent)
    : VideoSource(parent)
{
}

LocalVideoDevice::LocalVideoDevice(std::unique_ptr<LiveKitCpp::LocalVideoDevice> device,
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
        _device->resetFilter();
    }
}

void LocalVideoDevice::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (_device) {
        _device->setDeviceInfo(info);
    }
}

void LocalVideoDevice::setOptions(const VideoOptions& options)
{
    if (_device) {
        _device->setOptions(options);
    }
}

MediaDeviceInfo LocalVideoDevice::deviceInfo() const
{
    if (_device) {
        return _device->deviceInfo();
    }
    return {};
}

VideoOptions LocalVideoDevice::options() const
{
    if (_device) {
        return _device->options();
    }
    return {};
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
    if (_device) {
        if (subscribe) {
            _device->addSink(this);
        }
        else {
            _device->removeSink(this);
        }
    }
}

void LocalVideoDevice::applyFilter(VideoFilter* filter)
{
    VideoSource::applyFilter(filter);
    if (_device) {
        _device->setFilter(filter);
    }
}
