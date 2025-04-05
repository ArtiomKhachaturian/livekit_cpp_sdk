#ifndef DEMOAPP_H
#define DEMOAPP_H
#include <QGuiApplication>
#include <QScopedPointer>
#include <QPointer>
#include <optional>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace LiveKitCpp {
class Service;
enum class ServiceState;
}

class DemoApp : public QGuiApplication
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(bool audioRecordingEnabled READ audioRecordingEnabled WRITE setAudioRecordingEnabled NOTIFY audioRecordingChanged)
    Q_PROPERTY(bool audioPlayoutEnabled READ audioPlayoutEnabled WRITE setAudioPlayoutEnabled NOTIFY audioPlayoutChanged)
    Q_PROPERTY(int audioRecordingVolume READ audioRecordingVolume WRITE setAudioRecordingVolume NOTIFY audioRecordingVolumeChanged)
    Q_PROPERTY(int audioPlayoutVolume READ audioPlayoutVolume WRITE setAudioPlayoutVolume NOTIFY audioPlayoutVolumeChanged)
public:
    DemoApp(int &argc, char **argv);
    ~DemoApp() override;
public slots:
    void setAppWindow(QObject* appWindow, const QUrl&);
    Q_INVOKABLE void setAudioRecordingEnabled(bool enabled);
    Q_INVOKABLE void setAudioPlayoutEnabled(bool enabled);
    Q_INVOKABLE void setAudioRecordingVolume(int volume);
    Q_INVOKABLE void setAudioPlayoutVolume(int volume);
    Q_INVOKABLE bool registerClient(const QString& id);
    Q_INVOKABLE void unregisterClient(const QString& id);
public:
    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE bool audioRecordingEnabled() const;
    Q_INVOKABLE bool audioPlayoutEnabled() const;
    Q_INVOKABLE int audioRecordingVolume() const;
    Q_INVOKABLE int audioPlayoutVolume() const;
signals:
    void showErrorMessage(const QString& message);
    void audioRecordingChanged();
    void audioPlayoutChanged();
    void audioRecordingVolumeChanged();
    void audioPlayoutVolumeChanged();
private:
    QScopedPointer<LiveKitCpp::Service> _service;
    std::optional<LiveKitCpp::ServiceState> _serviceInitFailure;
    QPointer<QObject> _appWindow;
};

#endif // DEMOAPP_H
