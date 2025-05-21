#include "demoapp.h"
#include "logger.h"
#include "videofilter.h"
#include <ZaphoydTppFactory.h>
#include <livekit/rtc/Options.h>
#include <livekit/rtc/Service.h>
#include <livekit/rtc/media/WavFramesWriter.h>
#include <livekit/signaling/sfu/ICETransportPolicy.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
#include <livekit/rtc/media/qt/VideoFrameQtHelper.h>
#include <QQuickItem>
#include <QVideoSink>
#include <private/qquickpalette_p.h>
#endif
#include <QStandardPaths>
#include <memory>

namespace
{

inline double normalizeVolume(int volume) {
    volume = qBound(0, volume, 100);
    return volume / 100.;
}

inline double normalizedVolume(double volume) {
    volume = qBound<double>(0., volume, 1.);
    return qRound(volume * 100);
}

inline QString toQString(LiveKitCpp::IceTransportPolicy policy) {
    return QString::fromStdString(LiveKitCpp::toString(policy));
}

inline QStringList iceTransportPolicies() {
    using namespace LiveKitCpp;
    QStringList policies;
    for (auto policy : {IceTransportPolicy::None,
                        IceTransportPolicy::Relay,
                        IceTransportPolicy::NoHost,
                        IceTransportPolicy::All}) {
        policies.append(toQString(policy));
    }
    return policies;
}

}

DemoApp::DemoApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
    , _recordingAudioDevicesModel(new MediaDevicesModel(this))
    , _playoutAudioDevicesModel(new MediaDevicesModel(this))
    , _camerasModel(new MediaDevicesModel(this))
    , _iceTransportPolicies(iceTransportPolicies())
    , _defaultIceTransportPolicy(toQString(LiveKitCpp::Options()._iceTransportPolicy))
    , _defaultIceTransportPolicyIndex(_iceTransportPolicies.indexOf(_defaultIceTransportPolicy))
{
    LiveKitCpp::ServiceInitInfo initInfo;
    //initInfo._logger = std::make_shared<Logger>();
    auto service = std::make_shared<LiveKitCpp::Service>(std::make_shared<ZaphoydTppFactory>(), std::move(initInfo));
    const auto state = service->state();
    if (LiveKitCpp::ServiceState::OK == state) {
        _service = std::move(service);
        _recordingAudioDevice = _service->defaultAudioRecordingDevice();
        _playoutAudioDevice = _service->defaultAudioPlayoutDevice();
        _recordingAudioDevicesModel->setItems(_service->recordingAudioDevices());
        _playoutAudioDevicesModel->setItems(_service->playoutAudioDevices());
        _camerasModel->setItems(_service->cameraDevices());
        _service->addListener(this);
        const auto musicFolder = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        if (!musicFolder.isEmpty()) {
            const QString filenameSuffix = QDateTime::currentDateTime().toString() + ".wav";
            auto filename = musicFolder + "/" + "livekit_audio_rec_" + filenameSuffix;
            _recordingAudioWriter.reset(new LiveKitCpp::WavFramesWriter(filename.toStdString()));
            filename = musicFolder + "/" + "livekit_audio_play_" + filenameSuffix;
            _playoutAudioWriter.reset(new LiveKitCpp::WavFramesWriter(filename.toStdString()));
            _service->setRecordingFramesWriter(_recordingAudioWriter.data());
            _service->setPlayoutFramesWriter(_playoutAudioWriter.data());
        }
    }
    else {
        _serviceInitFailure = state;
    }
}

DemoApp::~DemoApp()
{
    if (_service) {
        _service->removeListener(this);
        _service->setRecordingFramesWriter(nullptr);
        _service->setPlayoutFramesWriter(nullptr);
    }
    _recordingAudioWriter.reset();
    _playoutAudioWriter.reset();
}

void DemoApp::setAppWindow(QObject* appWindow, const QUrl&)
{
    if (_appWindow != appWindow) {
        _appWindow = qobject_cast<QQuickWindow*>(appWindow);
        if (appWindow && _serviceInitFailure.has_value()) {
            switch (_serviceInitFailure.value()) {
                case LiveKitCpp::ServiceState::OK:
                    break;
                case LiveKitCpp::ServiceState::WSAFailure:
                    emit showErrorMessage(tr("Failed to intialize of WSA"));
                    break;
                case LiveKitCpp::ServiceState::SSLInitError:
                    emit showErrorMessage(tr("Failed to intialize of SSL"));
                    break;
                case LiveKitCpp::ServiceState::NoWebsoketsFactory:
                    emit showErrorMessage(tr("Websockets factory is not available"));
                    break;
                case LiveKitCpp::ServiceState::WebRTCInitError:
                    emit showErrorMessage(tr("Failed to intialize of WebRTC"));
                    break;
            }
        }
    }
}

void DemoApp::setAudioRecordingEnabled(bool enabled)
{
    if (_service) {
        _service->setAudioRecordingEnabled(enabled);
    }
}

void DemoApp::setAudioPlayoutEnabled(bool enabled)
{
    if (_service) {
        _service->setAudioPlayoutEnabled(enabled);
    }
}

void DemoApp::setAudioRecordingVolume(int volume)
{
    if (_service) {
        _service->setRecordingAudioVolume(normalizeVolume(volume));
    }
}

void DemoApp::setAudioPlayoutVolume(int volume)
{
    if (_service) {
        _service->setPlayoutAudioVolume(normalizeVolume(volume));
    }
}

void DemoApp::enableAudioRecordingProcessing(bool enable)
{
    if (_service && _service->audioRecordingProcessingEnabled() != enable) {
        _service->enableAudioRecordingProcessing(enable);
        emit audioRecordingProcessingChanged();
    }
}

void DemoApp::enableAudioPlayoutProcessing(bool enable)
{
    if (_service && _service->audioPlayoutProcessingEnabled() != enable) {
        _service->enableAudioPlayoutProcessing(enable);
        emit audioPlayoutProcessingChanged();
    }
}

void DemoApp::setRecordingAudioDevice(const MediaDeviceInfo& device)
{
    if (_service) {
        _service->setAudioRecordingDevice(device);
    }
}

void DemoApp::setPlayoutAudioDevice(const MediaDeviceInfo& device)
{
    if (_service) {
        _service->setAudioPlayoutDevice(device);
    }
}

QStringList DemoApp::availableFilters() const
{
    return VideoFilter::available();
}

bool DemoApp::displayCameraSettingsDialogBox(const MediaDeviceInfo& info,
                                             const QString& dialogTitle,
                                             uint32_t positionX, uint32_t positionY) const
{
    if (_service) {
        const auto dialogTitleUTF8 = dialogTitle.toStdString();
        WId parentWindow = _appWindow ? _appWindow->winId() : WId{};
        return _service->displayCameraSettingsDialogBox(info, dialogTitleUTF8,
                                                        reinterpret_cast<void*>(parentWindow),
                                                        positionX, positionY);
    }
    return false;
}

QStringList DemoApp::videoEncoders() const
{
    if (_service) {
        const auto utf8Encoders = _service->videoEncoderFormats();
        if (!utf8Encoders.empty()) {
            QStringList encoders;
            encoders.reserve(utf8Encoders.size());
            for (const auto& encoder : utf8Encoders) {
                encoders.append(QString::fromStdString(encoder));
            }
            return encoders;
        }
    }
    return {};
}

QStringList DemoApp::audioEncoders() const
{
    if (_service) {
        const auto utf8Encoders = _service->audioEncoderFormats();
        if (!utf8Encoders.empty()) {
            QStringList encoders;
            encoders.reserve(utf8Encoders.size());
            for (const auto& encoder : utf8Encoders) {
                encoders.append(QString::fromStdString(encoder));
            }
            return encoders;
        }
    }
    return {};
}

void DemoApp::clearVideoOutput(QObject* videoOutput) const
{
    if (videoOutput) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        QMetaObject::invokeMethod(videoOutput, "clearOutput");
#else
        if (const auto sink = videoOutput->property("videoSink").value<QVideoSink*>()) {
            const auto r = videoOutput->property("sourceRect").toRectF();
            if (!r.isEmpty()) {
                QImage image(r.size().toSize(), QImage::Format_ARGB32);
                QColor clearColor(Qt::transparent);
                const auto palette = videoOutput->property("palette");
                if (palette.canConvert<QQuickPalette*>()) {
                    clearColor = palette.value<QQuickPalette*>()->base();
                }
                image.fill(clearColor);
                if (auto frame = LiveKitCpp::QImageVideoFrame::create(std::move(image))) {
                    sink->setVideoFrame(LiveKitCpp::convert(std::move(frame)));
                }
            }
        }
#endif
    }
}

bool DemoApp::isValid() const
{
    return nullptr != _service;
}

bool DemoApp::audioRecordingEnabled() const
{
    return _service && _service->audioRecordingEnabled();
}

bool DemoApp::audioPlayoutEnabled() const
{
    return _service && _service->audioPlayoutEnabled();
}

bool DemoApp::audioRecordingProcessingEnabled()
{
    return _service && _service->audioRecordingProcessingEnabled();
}

bool DemoApp::audioPlayoutProcessingEnabled()
{
    return _service && _service->audioPlayoutProcessingEnabled();
}

int DemoApp::audioRecordingVolume() const
{
    if (_service) {
        return normalizedVolume(_service->recordingAudioVolume());
    }
    return 0;
}

int DemoApp::audioPlayoutVolume() const
{
    if (_service) {
        return normalizedVolume(_service->playoutAudioVolume());
    }
    return 0;
}

VideoOptions DemoApp::defaultCameraOptions() const
{
    return LiveKitCpp::Service::defaultCameraOptions();
}

void DemoApp::onAudioRecordingStarted()
{

}

void DemoApp::onAudioRecordingStopped()
{

}
void DemoApp::onAudioPlayoutStarted()
{

}

void DemoApp::onAudioPlayoutStopped()
{

}

void DemoApp::onAudioRecordingEnabled(bool)
{
    emit audioRecordingEnabledChanged();
}

void DemoApp::onAudioPlayoutEnabled(bool)
{
    emit audioPlayoutEnabledChanged();
}

void DemoApp::onAudioRecordingVolumeChanged(double)
{
    emit audioRecordingVolumeChanged();
}

void DemoApp::onAudioPlayoutVolumeChanged(double)
{
    emit audioRecordingVolumeChanged();
}

void DemoApp::onAudioRecordingDeviceChanged(const LiveKitCpp::MediaDeviceInfo& info)
{
    if (_recordingAudioDevice.exchange(info)) {
        emit recordingAudioDeviceChanged();
    }
}

void DemoApp::onAudioPlayoutDeviceChanged(const LiveKitCpp::MediaDeviceInfo& info)
{
    if (_playoutAudioDevice.exchange(info)) {
        emit playoutAudioDeviceChanged();
    }
}
