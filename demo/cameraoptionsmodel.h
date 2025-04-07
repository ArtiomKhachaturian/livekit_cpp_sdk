#ifndef CAMERAOPTIONSMODEL_H
#define CAMERAOPTIONSMODEL_H
#include "cameraoptions.h"
#include "mediadeviceinfo.h"
#include "itemmodel.h"
#include <memory>

namespace LiveKitCpp {
class Service;
}

class CameraOptionsModel : public ItemModel<CameraOptions>
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
    using Base = ItemModel<CameraOptions>;
public:
    CameraOptionsModel(const std::weak_ptr<LiveKitCpp::Service>& service = {},
                       QObject* parent = nullptr);
    Q_INVOKABLE MediaDeviceInfo deviceInfo() const { return _deviceInfo; }
    Q_INVOKABLE CameraOptions itemAt(qsizetype index) const;
    Q_INVOKABLE qsizetype indexOf(const CameraOptions& item) const;
public slots:
    void setDeviceInfo(const MediaDeviceInfo& info);
signals:
    void deviceInfoChanged();
private:
    const std::weak_ptr<LiveKitCpp::Service> _service;
    MediaDeviceInfo _deviceInfo;
};

#endif // CAMERAOPTIONSMODEL_H
