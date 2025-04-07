#include "cameraoptionsmodel.h"
#include <Service.h>

CameraOptionsModel::CameraOptionsModel(const std::weak_ptr<LiveKitCpp::Service>& service,
                                       QObject* parent)
    : Base(parent)
    , _service(service)
{
}

CameraOptions CameraOptionsModel::itemAt(qsizetype index) const
{
    return Base::itemAt(index);
}

qsizetype CameraOptionsModel::indexOf(const CameraOptions& item) const
{
    return Base::indexOf(item);
}

void CameraOptionsModel::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto service = _service.lock()) {
        if (info != _deviceInfo) {
            _deviceInfo = info;
            setItems(service->cameraOptions(info));
            emit deviceInfoChanged();
        }
    }
}
