#include "videomodels.h"
#include "demoapp.h"
#include "localvideodevice.h"
#include <livekit/rtc/Service.h>

namespace
{

inline std::weak_ptr<LiveKitCpp::Service> service() {
    if (const auto app = qobject_cast<DemoApp*>(QCoreApplication::instance())) {
        return app->service();
    }
    return {};
}

}

VideoSourcesModel::VideoSourcesModel(QObject* parent)
    : ItemModel{parent}
{
}


LocalVideoSourcesModel::LocalVideoSourcesModel(QObject* parent)
    : VideoSourcesModel(parent)
    , _service(service())
{

}

qsizetype LocalVideoSourcesModel::addCamera(const MediaDeviceInfo& info, const VideoOptions& options)
{
    if (const auto service = _service.lock()) {
        return addDevice(service->createCamera(info, options));
    }
    return -1;
}

qsizetype LocalVideoSourcesModel::addSharing(const MediaDeviceInfo& info, const VideoOptions& options)
{
    if (const auto service = _service.lock()) {
        return addDevice(service->createSharing(info, options));
    }
    return -1;
}

QList<MediaDeviceInfo> LocalVideoSourcesModel::screens() const
{
    if (const auto service = _service.lock()) {
        auto screens = service->screens();
        return QList<MediaDeviceInfo>(screens.begin(), screens.end());
    }
    return {};
}

QList<MediaDeviceInfo> LocalVideoSourcesModel::windows() const
{
    if (const auto service = _service.lock()) {
        auto windows = service->windows();
        return QList<MediaDeviceInfo>(windows.begin(), windows.end());
    }
    return {};
}

qsizetype LocalVideoSourcesModel::addDevice(std::shared_ptr<LiveKitCpp::LocalVideoDevice> sdkDevice)
{
    if (sdkDevice) {
        return append(new LocalVideoDevice(std::move(sdkDevice), this));
    }
    return -1;
}

ConnectionFormVideoModel::ConnectionFormVideoModel(QObject* parent)
    : LocalVideoSourcesModel(parent)
{
}

void ConnectionFormVideoModel::setActiveCamera(bool active)
{
    if (active != activeCamera()) {
        if (!active) {
            const auto sharingDev = itemAt(_sharingIndex);
            if (removeAt(_cameraIndex)) {
                setCameraIndex(-1);
                if (sharingDev) { // update index
                    setSharingIndex(indexOf(sharingDev));
                }
            }
        }
        else {
            setCameraIndex(addCamera(_cameraDeviceInfo));
        }
    }
}

void ConnectionFormVideoModel::setActiveSharing(bool active)
{
    if (active != activeSharing()) {
        if (!active) {
            const auto cameraDev = itemAt(_cameraIndex);
            if (removeAt(_sharingIndex)) {
                setSharingIndex(-1);
                if (cameraDev) { // update index
                    setCameraIndex(indexOf(cameraDev));
                }
            }
        }
        else {
            VideoOptions options;
            options.setPreview(true);
            options.setMaxFPS(5);
            setSharingIndex(addSharing(_sharingDeviceInfo, options));
        }
    }
}

void ConnectionFormVideoModel::setCameraDeviceInfo(const MediaDeviceInfo& info)
{
    if (info != _cameraDeviceInfo) {
        _cameraDeviceInfo = info;
        if (const auto device = qobject_cast<LocalVideoDevice*>(itemAt(_cameraIndex))) {
            device->setDeviceInfo(info);
        }
        emit cameraDeviceInfoChanged();
    }
}

void ConnectionFormVideoModel::setSharingDeviceInfo(const MediaDeviceInfo& info)
{
    if (info != _sharingDeviceInfo) {
        _sharingDeviceInfo = info;
        if (const auto device = qobject_cast<LocalVideoDevice*>(itemAt(_sharingIndex))) {
            device->setDeviceInfo(info);
        }
        emit sharingDeviceInfoChanged();
    }
}

void ConnectionFormVideoModel::setCameraIndex(qsizetype index)
{
    if (_cameraIndex != index) {
        _cameraIndex = index;
        emit activeCameraChanged();
    }
}

void ConnectionFormVideoModel::setSharingIndex(qsizetype index)
{
    if (_sharingIndex != index) {
        _sharingIndex = index;
        emit activeSharingChanged();
    }
}

SharingsVideoModel::SharingsVideoModel(QObject* parent)
    : LocalVideoSourcesModel(parent)
{
}

MediaDeviceInfo SharingsVideoModel::deviceInfo(qsizetype index) const
{
    if (const auto device = qobject_cast<LocalVideoDevice*>(itemAt(index))) {
        return device->deviceInfo();
    }
    return {};
}

void SharingsVideoModel::setMode(Mode mode)
{
    if (mode != _mode) {
        _mode = mode;
        clear();
        switch (mode) {
            case Mode::Inactive:
                break;
            case Mode::Screens:
            case Mode::Windows:
                resetContent();
                break;
            default:
                Q_ASSERT(false);
                break;
        }
    }
}

void SharingsVideoModel::resetContent()
{
    if (Mode::Inactive != _mode) {
        QList<MediaDeviceInfo> devices;
        VideoOptions options;
        options.setPreview(true);
        options.setResolution(640, 480);
        switch (_mode) {
            case Mode::Screens:
                devices = screens();
                options.setMaxFPS(5);
                break;
            case Mode::Windows:
                devices = windows();
                options.setMaxFPS(1);
                break;
            default:
                break;
        }
        for (qsizetype i = 0; i < devices.size(); ++i) {
            addSharing(devices[i], options);
        }
    }
}
