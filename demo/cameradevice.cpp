#include "cameradevice.h"
#include <livekit/media/CameraDevice.h>

CameraDevice::CameraDevice(std::shared_ptr<LiveKitCpp::CameraDevice> device,
                           QObject *parent)
    : VideoSource(parent)
    , _device(std::move(device))
{
    if (_device) {
        _device->addListener(this);
    }
}

CameraDevice::~CameraDevice()
{
    CameraDevice::subsribe(false);
    if (_device) {
        _device->removeListener(this);
    }
}

bool CameraDevice::isMuted() const
{
    return _device && _device->muted();
}

void CameraDevice::subsribe(bool subscribe)
{
    if (subscribe) {
        _device->addSink(this);
    }
    else {
        _device->removeSink(this);
    }
}
