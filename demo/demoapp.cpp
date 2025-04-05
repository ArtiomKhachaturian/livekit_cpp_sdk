#include "demoapp.h"
#include "logger.h"
#include <Service.h>
#include <ZaphoydTppFactory.h>
#include <QDebug>
#include <memory>

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

void DemoApp::registerClient(const QString& id, bool reg)
{
    if (_service && !id.isEmpty()) {
        qDebug() << id << (reg ? "is registered" : "is unregistered");
    }
}

bool DemoApp::isValid() const
{
    return nullptr != _service;
}
