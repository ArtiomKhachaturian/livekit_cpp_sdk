#ifndef LOCALVIDEODEVICE_H
#define LOCALVIDEODEVICE_H
#include "mediadeviceinfo.h"
#include "videooptions.h"
#include "videosource.h"
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class LocalVideoDevice;
}

class LocalVideoDevice : public VideoSource
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
public:
    explicit LocalVideoDevice(QObject *parent = nullptr);
    LocalVideoDevice(std::shared_ptr<LiveKitCpp::LocalVideoDevice> device, QObject *parent = nullptr);
    ~LocalVideoDevice() override;
    bool isValid() const { return nullptr != _device; }
    const auto& device() const noexcept { return _device; }
    Q_INVOKABLE void setDeviceInfo(const MediaDeviceInfo& info);
    Q_INVOKABLE void setOptions(const VideoOptions& options);
    MediaDeviceInfo deviceInfo() const;
    VideoOptions options() const;
    // overrides of VideoSource
    QString name() const final;
signals:
    void deviceInfoChanged();
    void optionsChanged();
protected:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMediaOptionsChanged(const std::string&) final { emit optionsChanged(); }
    void onMediaChanged(const std::string&) final { emit deviceInfoChanged(); }
    void onMuteChanged(const std::string&, bool) final {}
    // overrides of VideoSource
    bool hasVideoInput() const final { return isValid(); }
    bool isMuted() const final;
    void subsribe(bool subscribe) final;
private:
    const std::shared_ptr<LiveKitCpp::LocalVideoDevice> _device;
};

#endif // LOCALVIDEODEVICE_H
