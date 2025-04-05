#ifndef DEMOAPP_H
#define DEMOAPP_H
#include "safeobj.h"
#include "mediadeviceinfo.h"
#include "mediadevicesmodel.h"
#include <ServiceListener.h>
#include <QGuiApplication>
#include <QScopedPointer>
#include <QHash>
#include <QPointer>
#include <optional>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace LiveKitCpp {
class Service;
enum class ServiceState;
}

class SessionWrapper;

class DemoApp : public QGuiApplication, private LiveKitCpp::ServiceListener
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid CONSTANT)
    Q_PROPERTY(bool audioRecordingEnabled READ audioRecordingEnabled WRITE setAudioRecordingEnabled NOTIFY audioRecordingEnabledChanged)
    Q_PROPERTY(bool audioPlayoutEnabled READ audioPlayoutEnabled WRITE setAudioPlayoutEnabled NOTIFY audioPlayoutEnabledChanged)
    Q_PROPERTY(int audioRecordingVolume READ audioRecordingVolume WRITE setAudioRecordingVolume NOTIFY audioRecordingVolumeChanged)
    Q_PROPERTY(int audioPlayoutVolume READ audioPlayoutVolume WRITE setAudioPlayoutVolume NOTIFY audioPlayoutVolumeChanged)
    Q_PROPERTY(MediaDevicesModel* recordingAudioDevicesModel MEMBER _recordingAudioDevicesModel CONSTANT)
    Q_PROPERTY(MediaDevicesModel* playoutAudioDevicesModel MEMBER _playoutAudioDevicesModel CONSTANT)
    Q_PROPERTY(MediaDeviceInfo recordingAudioDevice READ recordingAudioDevice WRITE setRecordingAudioDevice NOTIFY recordingAudioDeviceChanged)
    Q_PROPERTY(MediaDeviceInfo playoutAudioDevice READ playoutAudioDevice WRITE setPlayoutAudioDevice NOTIFY playoutAudioDeviceChanged)
public:
    DemoApp(int &argc, char **argv);
    ~DemoApp() override;
public slots:
    void setAppWindow(QObject* appWindow, const QUrl&);
    Q_INVOKABLE void setAudioRecordingEnabled(bool enabled);
    Q_INVOKABLE void setAudioPlayoutEnabled(bool enabled);
    Q_INVOKABLE void setAudioRecordingVolume(int volume);
    Q_INVOKABLE void setAudioPlayoutVolume(int volume);
    Q_INVOKABLE void setRecordingAudioDevice(const MediaDeviceInfo& device);
    Q_INVOKABLE void setPlayoutAudioDevice(const MediaDeviceInfo& device);
    Q_INVOKABLE void unregisterClient(const QString& clientId);
    Q_INVOKABLE void connect(const QString& clientId, const QString& url, const QString& token);
public:
    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE bool audioRecordingEnabled() const;
    Q_INVOKABLE bool audioPlayoutEnabled() const;
    Q_INVOKABLE int audioRecordingVolume() const;
    Q_INVOKABLE int audioPlayoutVolume() const;
    Q_INVOKABLE MediaDeviceInfo recordingAudioDevice() const { return _recordingAudioDevice; }
    Q_INVOKABLE MediaDeviceInfo playoutAudioDevice() const { return _playoutAudioDevice; }
signals:
    void showErrorMessage(const QString& message,
                          const QString& details = {},
                          const QString& clientId = {});
    void audioRecordingEnabledChanged();
    void audioPlayoutEnabledChanged();
    void audioRecordingVolumeChanged();
    void audioPlayoutVolumeChanged();
    void recordingAudioDeviceChanged();
    void playoutAudioDeviceChanged();
private slots:
    void onSessionError(const QString& clientId, const QString& desc, const QString& details);
private:
    // impl. of LiveKitCpp::ServiceListener
    void onAudioRecordingStarted() final;
    void onAudioRecordingStopped() final;
    void onAudioPlayoutStarted() final;
    void onAudioPlayoutStopped() final;
    void onAudioRecordingEnabled(bool) final;
    void onAudioPlayoutEnabled(bool) final;
    void onAudioRecordingVolumeChanged(double) final;
    void onAudioPlayoutVolumeChanged(double) final;
    void onAudioRecordingDeviceChanged(const LiveKitCpp::MediaDeviceInfo& info) final;
    void onAudioPlayoutDeviceChanged(const LiveKitCpp::MediaDeviceInfo& info) final;
private:
    MediaDevicesModel* const _recordingAudioDevicesModel;
    MediaDevicesModel* const _playoutAudioDevicesModel;
    QScopedPointer<LiveKitCpp::Service> _service;
    std::optional<LiveKitCpp::ServiceState> _serviceInitFailure;
    QPointer<QObject> _appWindow;
    SafeObj<MediaDeviceInfo> _recordingAudioDevice;
    SafeObj<MediaDeviceInfo> _playoutAudioDevice;
    QHash<QString, SessionWrapper*> _clients;
};

#endif // DEMOAPP_H
