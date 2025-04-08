#include "cameradevicewrapper.h"
#include "media/CameraDevice.h"

CameraDeviceWrapper::CameraDeviceWrapper(std::shared_ptr<LiveKitCpp::CameraDevice> device,
                                         QObject *parent)
    : VideoSinkWrapper(parent)
    , _device(std::move(device))
{
    if (_device) {
        _device->addListener(this);
    }
}

CameraDeviceWrapper::~CameraDeviceWrapper()
{
    CameraDeviceWrapper::subsribe(false);
    if (_device) {
        _device->removeListener(this);
    }
}

bool CameraDeviceWrapper::isMuted() const
{
    return _device && _device->muted();
}

void CameraDeviceWrapper::subsribe(bool subscribe)
{
    if (subscribe) {
        _device->addSink(this);
    }
    else {
        _device->removeSink(this);
    }
}
