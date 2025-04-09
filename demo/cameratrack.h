#ifndef CAMERATRACK_H
#define CAMERATRACK_H
#include "mediadeviceinfo.h"
#include "cameraoptions.h"
#include "videotrack.h"

namespace LiveKitCpp {
class CameraTrack;
}

class CameraTrack : public VideoTrack
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CameraTrack)
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
    Q_PROPERTY(CameraOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
public:
    explicit CameraTrack(QObject *parent = nullptr);
    CameraTrack(const std::shared_ptr<LiveKitCpp::CameraTrack>& impl, QObject *parent = nullptr);
    Q_INVOKABLE MediaDeviceInfo deviceInfo() const;
    Q_INVOKABLE CameraOptions options() const;
public slots:
    void setDeviceInfo(const MediaDeviceInfo& info = {});
    void setOptions(const CameraOptions& options);
signals:
    void deviceInfoChanged();
    void optionsChanged();
private:
    // impl. of LiveKitCpp::CameraEventsListener
    void onCapturerChanged(const std::string&, const LiveKitCpp::MediaDeviceInfo&) final;
    void onOptionsChanged(const std::string&, const LiveKitCpp::CameraOptions&) final;
private:
    const std::weak_ptr<LiveKitCpp::CameraTrack> _impl;
};

Q_DECLARE_METATYPE(CameraTrack*)

#endif // CAMERATRACK_H
