#ifndef CAMERATRACKWRAPPER_H
#define CAMERATRACKWRAPPER_H
#include "mediadeviceinfo.h"
#include "videotrackwrapper.h"

namespace LiveKitCpp {
class CameraTrack;
}

class CameraTrackWrapper : public VideoTrackWrapper
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CameraTrackWrapper)
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
public:
    CameraTrackWrapper(const std::shared_ptr<LiveKitCpp::CameraTrack>& impl = {},
                       QObject *parent = nullptr);
    Q_INVOKABLE MediaDeviceInfo deviceInfo() const;
public slots:
    void setDeviceInfo(const MediaDeviceInfo& info = {});
signals:
    void deviceInfoChanged();
private:
    const std::weak_ptr<LiveKitCpp::CameraTrack> _impl;
};

Q_DECLARE_METATYPE(CameraTrackWrapper*)

#endif // CAMERATRACKWRAPPER_H
