#ifndef CAMERAOPTIONSMODEL_H
#define CAMERAOPTIONSMODEL_H
#include "videooptions.h"
#include "mediadeviceinfo.h"
#include "itemmodel.h"
#include <memory>

namespace LiveKitCpp {
class Service;
}

class CameraOptionsModel : public ItemModel<VideoOptions>
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(VideoOptions defaultOptions READ defaultOptions CONSTANT)
public:
    explicit CameraOptionsModel(QObject* parent = nullptr);
    const MediaDeviceInfo& deviceInfo() const noexcept { return _deviceInfo; }
    Q_INVOKABLE VideoOptions itemAt(qsizetype index) const;
    Q_INVOKABLE qsizetype indexOf(const VideoOptions& item) const;
    bool isValid() const { return !_service.expired(); }
    VideoOptions defaultOptions() const;
    Q_INVOKABLE qsizetype defaultOptionsIndex() const;
public slots:
    void setDeviceInfo(const MediaDeviceInfo& info);
signals:
    void deviceInfoChanged();
private:
    const std::weak_ptr<LiveKitCpp::Service> _service;
    MediaDeviceInfo _deviceInfo;
};

#endif // CAMERAOPTIONSMODEL_H
