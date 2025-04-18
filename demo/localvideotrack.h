#ifndef LOCALVIDEOTRACK_H
#define LOCALVIDEOTRACK_H
#include "mediadeviceinfo.h"
#include "videooptions.h"
#include "videotrack.h"

namespace LiveKitCpp {
class LocalVideoTrack;
}

class LocalVideoTrack : public VideoTrack
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LocalVideoTrack)
    Q_PROPERTY(MediaDeviceInfo deviceInfo READ deviceInfo WRITE setDeviceInfo NOTIFY deviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
public:
    explicit LocalVideoTrack(QObject *parent = nullptr);
    LocalVideoTrack(const std::shared_ptr<LiveKitCpp::LocalVideoTrack>& impl, QObject *parent = nullptr);
    MediaDeviceInfo deviceInfo() const;
    VideoOptions options() const;
public slots:
    void setDeviceInfo(const MediaDeviceInfo& info = {});
    void setOptions(const VideoOptions& options);
signals:
    void deviceInfoChanged();
    void optionsChanged();
private:
    // impl. of LiveKitCpp::CameraEventsListener
    void onMediaChanged(const std::string&) final;
    void onMediaOptionsChanged(const std::string&) final;
private:
    const std::weak_ptr<LiveKitCpp::LocalVideoTrack> _impl;
};

#endif // LOCALVIDEOTRACK_H
