#include "demoapp.h"
#include "logger.h"
#include "videofilter.h"
#include <livekit/rtc/Options.h>
#include <livekit/rtc/Service.h>
#include <livekit/signaling/sfu/ICETransportPolicy.h>
#include <ZaphoydTppFactory.h>
#include <QDebug>
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
    const auto logger = std::make_shared<Logger>();
    const auto wsf = std::make_shared<ZaphoydTppFactory>();
    auto service = std::make_shared<LiveKitCpp::Service>(wsf/*, logger*/);
    const auto state = service->state();
    if (LiveKitCpp::ServiceState::OK == state) {
        _service = std::move(service);
        _recordingAudioDevice = _service->defaultAudioRecordingDevice();
        _playoutAudioDevice = _service->defaultAudioPlayoutDevice();
        _recordingAudioDevicesModel->setItems(_service->recordingAudioDevices());
        _playoutAudioDevicesModel->setItems(_service->playoutAudioDevices());
        _camerasModel->setItems(_service->cameraDevices());
        _service->addListener(this);
    }
    else {
        _serviceInitFailure = state;
    }
}

DemoApp::~DemoApp()
{
    if (_service) {
        _service->removeListener(this);
    }
}

void DemoApp::setAppWindow(QObject* appWindow, const QUrl&)
{
    if (_appWindow != appWindow) {
        _appWindow = appWindow;
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
