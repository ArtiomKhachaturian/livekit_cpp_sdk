#ifndef CAMERADEVICE_H
#define CAMERADEVICE_H
#include "mediadeviceinfo.h"
#include "videosource.h"
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class CameraDevice;
}

class CameraDevice : public VideoSource
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
public:
    CameraDevice(std::shared_ptr<LiveKitCpp::CameraDevice> device = {},
                 QObject *parent = nullptr);
    ~CameraDevice() override;
    bool isValid() const { return nullptr != _device; }
    const auto& device() const noexcept { return _device; }
    Q_INVOKABLE void setDeviceInfo(const MediaDeviceInfo& info);
protected:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool) final {}
    // overrides of VideoSource
    bool hasVideoInput() const final { return isValid(); }
    bool isMuted() const final;
    void subsribe(bool subscribe) final;
private:
    const std::shared_ptr<LiveKitCpp::CameraDevice> _device;
};

#endif // CAMERADEVICE_H
