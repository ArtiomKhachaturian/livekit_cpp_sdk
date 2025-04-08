#ifndef CAMERADEVICEWRAPPER_H
#define CAMERADEVICEWRAPPER_H
#include "videosinkwrapper.h"
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class CameraDevice;
}

class CameraDeviceWrapper : public VideoSinkWrapper
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
public:
    CameraDeviceWrapper(std::shared_ptr<LiveKitCpp::CameraDevice> device = {},
                        QObject *parent = nullptr);
    ~CameraDeviceWrapper() override;
    Q_INVOKABLE bool isValid() const { return nullptr != _device; }
    const auto& device() const noexcept { return _device; }
protected:
    // overrides of VideoSinkWrapper
    bool hasVideoInput() const final { return isValid(); }
    bool isMuted() const final;
    void subsribe(bool subscribe) final;
private:
    const std::shared_ptr<LiveKitCpp::CameraDevice> _device;
};

#endif // CAMERADEVICEWRAPPER_H
