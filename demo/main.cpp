#include "demoapp.h"
#include "remoteparticipant.h"
#include "mediadeviceinfo.h"
#include "audiorecordingoptions.h"
#include "videooptions.h"
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    qRegisterMetaType<RemoteParticipant*>();
    qRegisterMetaType<MediaDeviceInfo>();
    qRegisterMetaType<VideoOptions>();
    qRegisterMetaType<AudioRecordingOptions>();

    DemoApp app(argc, argv);
    QQmlApplicationEngine engine;

    //Load the style
    //QQuickStyle::setStyle("Material");
    //QQuickStyle::setStyle("Universal");
    QQuickStyle::setStyle("Fusion");
    // QQuickStyle::setStyle("Imagine");
    //QQuickStyle::setStyle("Default");

    engine.rootContext()->setContextProperty("app", &app);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, &DemoApp::setAppWindow, Qt::QueuedConnection);
    engine.loadFromModule("LiveKitClient", "Main");

    return app.exec();
}
