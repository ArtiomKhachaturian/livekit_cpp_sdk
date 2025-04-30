#ifndef VIDEOTRACK_H
#define VIDEOTRACK_H
#include "videosource.h"
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrack : public VideoSource
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrack)
public:
    enum NetworkPriority
    {
        VeryLow,
        Low,
        Medium,
        High,
    };
    enum ContentHint
    {
        None,
        Fluid,
        Detailed,
        Text
    };
    enum DegradationPreference
    {
        Default,
        Disabled,
        MaintainFramerate,
        MaintainResolution,
        Balanced,
    };
public:
    Q_ENUM(NetworkPriority)
    Q_ENUM(ContentHint)
    Q_ENUM(DegradationPreference)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY muteChanged)
    Q_PROPERTY(bool screencast READ isScreencast CONSTANT)
    Q_PROPERTY(bool remote READ isRemote CONSTANT)
    Q_PROPERTY(NetworkPriority networkPriority READ networkPriority WRITE setNetworkPriority NOTIFY networkPriorityChanged)
    Q_PROPERTY(ContentHint contentHint READ contentHint WRITE setContentHint NOTIFY contentHintChanged)
    Q_PROPERTY(DegradationPreference degradationPreference READ degradationPreference WRITE setDegradationPreference NOTIFY degradationPreferenceChanged)
public:
    explicit VideoTrack(QObject* parent = nullptr);
    VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack,
               QObject* parent = nullptr);
    ~VideoTrack() override;
    std::shared_ptr<LiveKitCpp::VideoTrack> takeSdkTrack();
    QString id() const;
    bool muted() const;
    bool isScreencast() const;
    bool isRemote() const;
    NetworkPriority networkPriority() const;
    ContentHint contentHint() const;
    DegradationPreference degradationPreference() const;
public slots:
    void setMuted(bool muted);
    void setNetworkPriority(NetworkPriority priority);
    void setContentHint(ContentHint hint);
    void setDegradationPreference(DegradationPreference preference);
signals:
    void muteChanged();
    void networkPriorityChanged();
    void contentHintChanged();
    void degradationPreferenceChanged();
protected:
    // overrides of VideoSource
    bool hasVideoInput() const final { return nullptr != _sdkTrack; }
    bool isMuted() const final { return muted(); }
    void subsribe(bool subscribe) final;
private:
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool muted) final;
    void onRemoteSideMuteChanged(const std::string&, bool) final;
private:
    std::shared_ptr<LiveKitCpp::VideoTrack> _sdkTrack;
};

#endif // VIDEOTRACK_H
