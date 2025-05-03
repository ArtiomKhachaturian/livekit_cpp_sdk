#ifndef DEMOAPP_H
#define DEMOAPP_H
#include "safeobj.h"
#include "mediadevicesmodel.h"
#include "audiorecordingoptions.h"
#include "videooptions.h"
#include <livekit/rtc/ServiceListener.h>
#include <QGuiApplication>
#include <QStringList>
#include <QScopedPointer>
#include <QQuickWindow>
#include <QPointer>
#include <memory>
#include <optional>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace LiveKitCpp {
class Service;
enum class ServiceState;
}

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
    Q_PROPERTY(MediaDevicesModel* camerasModel MEMBER _camerasModel CONSTANT)
    Q_PROPERTY(MediaDeviceInfo recordingAudioDevice READ recordingAudioDevice WRITE setRecordingAudioDevice NOTIFY recordingAudioDeviceChanged)
    Q_PROPERTY(MediaDeviceInfo playoutAudioDevice READ playoutAudioDevice WRITE setPlayoutAudioDevice NOTIFY playoutAudioDeviceChanged)
    Q_PROPERTY(QStringList iceTransportPolicies MEMBER _iceTransportPolicies CONSTANT)
    Q_PROPERTY(QString defaultIceTransportPolicy MEMBER _defaultIceTransportPolicy CONSTANT)
    Q_PROPERTY(int defaultIceTransportPolicyIndex MEMBER _defaultIceTransportPolicyIndex CONSTANT)
    Q_PROPERTY(VideoOptions defaultCameraOptions READ defaultCameraOptions CONSTANT)
public:
    DemoApp(int &argc, char **argv);
    ~DemoApp() override;
    std::weak_ptr<LiveKitCpp::Service> service() const { return _service; }
    Q_INVOKABLE MediaDeviceInfo emptyDeviceInfo() const { return {}; }
    Q_INVOKABLE VideoOptions emptyVideoOptions() const { return {}; }
    Q_INVOKABLE AudioRecordingOptions emptyAudioRecordingOptions() const { return {}; }
    Q_INVOKABLE QStringList availableFilters() const;
    Q_INVOKABLE bool displayCameraSettingsDialogBox(const MediaDeviceInfo& info,
                                                    const QString& dialogTitle,
                                                    uint32_t positionX, uint32_t positionY) const;
    Q_INVOKABLE QStringList videoEncoders() const;
    Q_INVOKABLE QStringList audioEncoders() const;
    Q_INVOKABLE void clearVideoOutput(QObject* videoOutput) const;
public slots:
    void setAppWindow(QObject* appWindow, const QUrl&);
    void setAudioRecordingEnabled(bool enabled);
    void setAudioPlayoutEnabled(bool enabled);
    void setAudioRecordingVolume(int volume);
    void setAudioPlayoutVolume(int volume);
    void setRecordingAudioDevice(const MediaDeviceInfo& device);
    void setPlayoutAudioDevice(const MediaDeviceInfo& device);
public:
    bool isValid() const;
    bool audioRecordingEnabled() const;
    bool audioPlayoutEnabled() const;
    int audioRecordingVolume() const;
    int audioPlayoutVolume() const;
    MediaDeviceInfo recordingAudioDevice() const { return _recordingAudioDevice; }
    MediaDeviceInfo playoutAudioDevice() const { return _playoutAudioDevice; }
    VideoOptions defaultCameraOptions() const;
signals:
    void showErrorMessage(const QString& message, const QString& details = {});
    void audioRecordingEnabledChanged();
    void audioPlayoutEnabledChanged();
    void audioRecordingVolumeChanged();
    void audioPlayoutVolumeChanged();
    void recordingAudioDeviceChanged();
    void playoutAudioDeviceChanged();
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
    MediaDevicesModel* const _camerasModel;
    const QStringList _iceTransportPolicies;
    const QString _defaultIceTransportPolicy;
    const int _defaultIceTransportPolicyIndex;
    std::shared_ptr<LiveKitCpp::Service> _service;
    std::optional<LiveKitCpp::ServiceState> _serviceInitFailure;
    QPointer<QQuickWindow> _appWindow;
    SafeObj<MediaDeviceInfo> _recordingAudioDevice;
    SafeObj<MediaDeviceInfo> _playoutAudioDevice;
};

#endif // DEMOAPP_H
