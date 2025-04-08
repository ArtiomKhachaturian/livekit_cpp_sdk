#ifndef CAMERATRACKWRAPPER_H
#define CAMERATRACKWRAPPER_H
#include "mediadeviceinfo.h"
#include "cameraoptions.h"
#include "videosinkwrapper.h"

namespace LiveKitCpp {
class CameraTrack;
}

class CameraTrackWrapper : public VideoSinkWrapper
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CameraTrackWrapper)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
    Q_PROPERTY(CameraOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
public:
    CameraTrackWrapper(const std::shared_ptr<LiveKitCpp::CameraTrack>& impl = {},
                       QObject *parent = nullptr);
    ~CameraTrackWrapper() override;
    std::shared_ptr<LiveKitCpp::CameraTrack> track() const { return _impl.lock(); }
    Q_INVOKABLE MediaDeviceInfo deviceInfo() const;
    Q_INVOKABLE CameraOptions options() const;
    Q_INVOKABLE QString id() const;
    Q_INVOKABLE bool muted() const;
public slots:
    void setMuted(bool muted);
    void setDeviceInfo(const MediaDeviceInfo& info = {});
    void setOptions(const CameraOptions& options);
signals:
    void muteChanged();
    void deviceInfoChanged();
    void optionsChanged();
protected:
    // overrides of VideoSinkWrapper
    bool hasVideoInput() const final { return !_impl.expired(); }
    bool isMuted() const final { return muted(); }
    void subsribe(bool subscribe) final;
private:
    // impl. of LiveKitCpp::CameraEventsListener
    void onMuteChanged(const std::string&, bool) final { emit muteChanged(); }
    void onCapturerChanged(const std::string&, const LiveKitCpp::MediaDeviceInfo&) final;
    void onOptionsChanged(const std::string&, const LiveKitCpp::CameraOptions&) final;
private:
    const std::weak_ptr<LiveKitCpp::CameraTrack> _impl;
};

Q_DECLARE_METATYPE(CameraTrackWrapper*)

#endif // CAMERATRACKWRAPPER_H
