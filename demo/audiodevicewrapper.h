#ifndef AUDIODEVICEWRAPPER_H
#define AUDIODEVICEWRAPPER_H
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class AudioDevice;
}

class AudioDeviceWrapper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
public:
    AudioDeviceWrapper(std::shared_ptr<LiveKitCpp::AudioDevice> device = {},
                       QObject *parent = nullptr);
    ~AudioDeviceWrapper() override;
    Q_INVOKABLE bool isValid() const { return nullptr != _device; }
    const auto& device() const noexcept { return _device; }
private:
    const std::shared_ptr<LiveKitCpp::AudioDevice> _device;
};

#endif // AUDIODEVICEWRAPPER_H
