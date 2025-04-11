#include "cameraoptionsmodel.h"
#include "demoapp.h"
#include <livekit/rtc/Service.h>

namespace
{

inline std::weak_ptr<LiveKitCpp::Service> service() {
    if (const auto app = qobject_cast<DemoApp*>(QCoreApplication::instance())) {
        return app->service();
    }
    return {};
}

}

CameraOptionsModel::CameraOptionsModel(QObject* parent)
    : ItemModel<CameraOptions>(parent)
    , _service(service())
{
}

CameraOptions CameraOptionsModel::itemAt(qsizetype index) const
{
    return ItemModel<CameraOptions>::itemAt(index);
}

qsizetype CameraOptionsModel::indexOf(const CameraOptions& item) const
{
    return ItemModel<CameraOptions>::indexOf(item);
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

qsizetype CameraOptionsModel::defaultOptionsIndex() const
{
    return qMax<qsizetype>(0, indexOf(defaultOptions()));
}
