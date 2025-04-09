#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class AudioDevice;
}

class AudioDevice : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
public:
    AudioDevice(std::shared_ptr<LiveKitCpp::AudioDevice> device = {},
                QObject *parent = nullptr);
    ~AudioDevice() override;
    bool isValid() const { return nullptr != _device; }
    const auto& device() const noexcept { return _device; }
private:
    const std::shared_ptr<LiveKitCpp::AudioDevice> _device;
};

Q_DECLARE_METATYPE(AudioDevice*)

#endif // AUDIODEVICE_H
