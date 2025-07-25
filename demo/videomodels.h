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
    Q_INVOKABLE qsizetype addSharing(bool previewMode,
                                     const MediaDeviceInfo& info,
                                     const VideoOptions& options = {});
    Q_INVOKABLE bool removeDevice(qsizetype index) { return removeAt(index); }
    Q_INVOKABLE QList<MediaDeviceInfo> screens() const;
    Q_INVOKABLE QList<MediaDeviceInfo> windows() const;
private:
    qsizetype addDevice(std::unique_ptr<LiveKitCpp::LocalVideoDevice> device);
private:
    const std::weak_ptr<LiveKitCpp::Service> _service;
};

class ConnectionFormVideoModel : public LocalVideoSourcesModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool activeCamera READ activeCamera WRITE setActiveCamera NOTIFY activeCameraChanged FINAL)
    Q_PROPERTY(bool activeSharing READ activeSharing WRITE setActiveSharing NOTIFY activeSharingChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo cameraDeviceInfo READ cameraDeviceInfo WRITE setCameraDeviceInfo NOTIFY cameraDeviceInfoChanged FINAL)
    Q_PROPERTY(MediaDeviceInfo sharingDeviceInfo READ sharingDeviceInfo WRITE setSharingDeviceInfo NOTIFY sharingDeviceInfoChanged FINAL)
    Q_PROPERTY(VideoOptions cameraOptions READ cameraOptions WRITE setCameraOptions NOTIFY cameraOptionsChanged FINAL)
    Q_PROPERTY(VideoOptions sharingOptions READ sharingOptions WRITE setSharingOptions NOTIFY sharingOptionsChanged FINAL)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged FINAL)
private:
    struct DeviceData
    {
        bool _active = false;
        QPointer<LocalVideoDevice> _source;
        MediaDeviceInfo _info;
        VideoOptions _options;
    };
public:
    explicit ConnectionFormVideoModel(QObject* parent = nullptr);
    bool active() const { return _active; }
    bool activeCamera() const { return _camera._active; }
    bool activeSharing() const { return _sharing._active; }
    const auto& cameraDeviceInfo() const noexcept { return _camera._info; }
    const auto& sharingDeviceInfo() const noexcept { return _sharing._info; }
    const auto& cameraOptions() const noexcept { return _camera._options; }
    const auto& sharingOptions() const noexcept { return _sharing._options; }
    const auto& filter() const noexcept { return _filter; }
public slots:
    void setActive(bool active);
    void setActiveCamera(bool active);
    void setActiveSharing(bool active);
    void setCameraDeviceInfo(const MediaDeviceInfo& info);
    void setSharingDeviceInfo(const MediaDeviceInfo& info);
    void setCameraOptions(const VideoOptions& options);
    void setSharingOptions(const VideoOptions& options);
    void setFilter(const QString& filter);
signals:
    void activeCameraChanged();
    void activeSharingChanged();
    void cameraDeviceInfoChanged();
    void cameraOptionsChanged();
    void sharingDeviceInfoChanged();
    void sharingOptionsChanged();
    void activeChanged();
    void filterChanged();
private:
    template <typename TSignal>
    void setDeviceActive(DeviceData* data, bool active, TSignal signal);
    template <typename TSignal>
    void setDeviceInfo(DeviceData* data, const MediaDeviceInfo& info, TSignal signal);
    template <typename TSignal>
    void setOptions(DeviceData* data, const VideoOptions& options, TSignal signal);
    void addSource(DeviceData* data);
    void removeSource(DeviceData* data);
private:
    bool _active = false;
    DeviceData _camera;
    DeviceData _sharing;
    QString _filter;
};

class SharingsVideoModel : public LocalVideoSourcesModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(VideoOptions options READ options WRITE setOptions NOTIFY optionsChanged FINAL)
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
    const VideoOptions& options() const noexcept { return _options; }
    Q_INVOKABLE MediaDeviceInfo deviceInfo(qsizetype index) const;
public slots:
    void setMode(Mode mode);
    void setOptions(const VideoOptions& options);
signals:
    void modeChanged();
    void optionsChanged();
private:
    void resetContent();
private:
    Mode _mode = Mode::Inactive;
    VideoOptions _options;
};

#endif // VIDEOMODELS_H
