#ifndef VIDEOMODELS_H
#define VIDEOMODELS_H
#include "itemmodel.h"
#include "mediadeviceinfo.h"
#include "localvideodevice.h"
#include "videooptions.h"

namespace LiveKitCpp {
class LocalVideoDevice;
class Service;
}

class VideoSourcesModel : public ItemModel<VideoSource*>
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit VideoSourcesModel(QObject* parent = nullptr);
    Q_INVOKABLE VideoSource* sourceAt(int index) const { return itemAt(index); }
protected:
    QByteArray itemRoleName() const final { return QByteArrayLiteral("source"); }
};

class LocalVideoSourcesModel : public VideoSourcesModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LocalVideoSourcesModel(QObject* parent = nullptr);
    Q_INVOKABLE qsizetype addCamera(const MediaDeviceInfo& info = {},
                                    const VideoOptions& options = {});
    Q_INVOKABLE qsizetype addSharing(const MediaDeviceInfo& info,
                                     const VideoOptions& options = {});
    Q_INVOKABLE bool removeDevice(qsizetype index) { return removeAt(index); }
    Q_INVOKABLE QList<MediaDeviceInfo> screens() const;
    Q_INVOKABLE QList<MediaDeviceInfo> windows() const;
private:
    qsizetype addDevice(std::shared_ptr<LiveKitCpp::LocalVideoDevice> device);
private:
    const std::weak_ptr<LiveKitCpp::Service> _service;
};

class ConnectionFormVideoModel : public LocalVideoSourcesModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool activeCamera READ activeCamera WRITE setActiveCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(bool activeSharing READ activeSharing WRITE setActiveSharing NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo sharingDeviceInfo READ sharingDeviceInfo WRITE setSharingDeviceInfo NOTIFY sharingDeviceInfoChanged FINAL)
public:
    explicit ConnectionFormVideoModel(QObject* parent = nullptr);
    bool activeCamera() const { return -1 != _cameraIndex; }
    bool activeSharing() const { return -1 != _sharingIndex; }
    const MediaDeviceInfo& cameraDeviceInfo() const noexcept { return _cameraDeviceInfo; }
    const MediaDeviceInfo& sharingDeviceInfo() const noexcept { return _sharingDeviceInfo; }
public slots:
    void setActiveCamera(bool active);
    void setActiveSharing(bool active);
    void setCameraDeviceInfo(const MediaDeviceInfo& info);
    void setSharingDeviceInfo(const MediaDeviceInfo& info);
signals:
    void activeCameraChanged();
    void activeSharingChanged();
    void cameraDeviceInfoChanged();
    void sharingDeviceInfoChanged();
private:
    void setCameraIndex(qsizetype index);
    void setSharingIndex(qsizetype index);
private:
    qsizetype _cameraIndex = -1;
    qsizetype _sharingIndex = -1;
    MediaDeviceInfo _cameraDeviceInfo;
    MediaDeviceInfo _sharingDeviceInfo;
};

class SharingsVideoModel : public LocalVideoSourcesModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
public:
    enum Mode
    {
        Inactive,
        Screens,
        Windows,
    };
    Q_ENUM(Mode)
public:
    explicit SharingsVideoModel(QObject* parent = nullptr);
    Mode mode() const noexcept { return _mode; }
    Q_INVOKABLE MediaDeviceInfo deviceInfo(qsizetype index) const;
public slots:
    void setMode(Mode mode);
signals:
    void modeChanged();
private:
    void resetContent();
private:
    Mode _mode = Mode::Inactive;
    VideoOptions _screenOptions;
    VideoOptions _windowsOptions;
};

#endif // VIDEOMODELS_H
