#include "demoapp.h"
#include "logger.h"
#include <Service.h>
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

}

DemoApp::DemoApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
{
    const auto logger = std::make_shared<Logger>();
    const auto wsf = std::make_shared<ZaphoydTppFactory>(logger);
    auto service = std::make_unique<LiveKitCpp::Service>(wsf, logger);
    const auto state = service->state();
    if (LiveKitCpp::ServiceState::OK == state) {
         _service.reset(service.release());
    }
    else {
        _serviceInitFailure = state;
    }
}

DemoApp::~DemoApp()
{
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
                case LiveKitCpp::ServiceState::NoWebRTC:
                    emit showErrorMessage(tr("WebRTC is not available"));
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
        _service->setAudioRecording(enabled);
    }
}

void DemoApp::setAudioPlayoutEnabled(bool enabled)
{
    if (_service) {
        _service->setAudioPlayout(enabled);
    }
}

void DemoApp::setAudioRecordingVolume(int volume)
{
    if (_service) {
        _service->setRecordingVolume(normalizeVolume(volume));
    }
}

void DemoApp::setAudioPlayoutVolume(int volume)
{
    if (_service) {
        _service->setPlayoutVolume(normalizeVolume(volume));
    }
}

bool DemoApp::registerClient(const QString& id)
{
    if (_service && !id.isEmpty()) {
        //qDebug() << id << (reg ? "is registered" : "is unregistered");

        return true;
    }
    return false;
}

void DemoApp::unregisterClient(const QString& id)
{
    if (!id.isEmpty()) {

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

int DemoApp::audioRecordingVolume() const
{
    if (_service) {
        return normalizedVolume(_service->recordingVolume());
    }
    return 0;
}

int DemoApp::audioPlayoutVolume() const
{
    if (_service) {
        return normalizedVolume(_service->playoutVolume());
    }
    return 0;
}
