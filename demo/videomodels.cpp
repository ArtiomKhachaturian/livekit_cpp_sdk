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

void ConnectionFormVideoModel::setActive(bool active)
{
    if (active != _active) {
        _active = active;
        if (active) {
            addSource(&_camera);
            addSource(&_sharing);
        }
        else {
            removeSource(&_camera);
            removeSource(&_sharing);
        }
        emit activeChanged();
    }
}

void ConnectionFormVideoModel::setActiveCamera(bool active)
{
    setDeviceActive(&_camera, active, &ConnectionFormVideoModel::activeCameraChanged);
}

void ConnectionFormVideoModel::setActiveSharing(bool active)
{
    setDeviceActive(&_sharing, active, &ConnectionFormVideoModel::activeSharingChanged);
}

void ConnectionFormVideoModel::setCameraDeviceInfo(const MediaDeviceInfo& info)
{
    setDeviceInfo(&_camera, info, &ConnectionFormVideoModel::cameraDeviceInfoChanged);
}

void ConnectionFormVideoModel::setSharingDeviceInfo(const MediaDeviceInfo& info)
{
    setDeviceInfo(&_sharing, info, &ConnectionFormVideoModel::sharingDeviceInfoChanged);
}

template <typename TSignal>
void ConnectionFormVideoModel::setDeviceActive(DeviceData* data, bool active, TSignal signal)
{
    if (data && data->_active != active) {
        data->_active = active;
        if (_active) {
            if (active) {
                addSource(data);
            }
            else {
                removeSource(data);
            }
        }
        emit ((*this).*signal)();
    }
}

template <typename TSignal>
void ConnectionFormVideoModel::setDeviceInfo(DeviceData* data, const MediaDeviceInfo& info, TSignal signal)
{
    if (data && data->_info != info) {
        data->_info = info;
        if (data->_source) {
            data->_source->setDeviceInfo(info);
        }
        emit ((*this).*signal)();
    }
}

void ConnectionFormVideoModel::addSource(DeviceData* data)
{
    if (data && data->_active && !data->_source) {
        qsizetype index = -1;
        if (&_sharing == data) {
            VideoOptions options;
            options.setPreview(true);
            options.setMaxFPS(5);
            index = addSharing(data->_info, options);
        }
        else if (&_camera == data) {
            index = addCamera(data->_info);
        }
        else {
            Q_ASSERT(false);
        }
        if (-1 != index) {
            data->_source = dynamic_cast<LocalVideoDevice*>(itemAt(index));
        }
    }
}

void ConnectionFormVideoModel::removeSource(DeviceData* data)
{
    if (data && data->_source) {
        remove(data->_source);
        data->_source = nullptr;
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
