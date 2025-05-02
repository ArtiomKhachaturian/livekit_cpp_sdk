#ifndef VIDEOTRACK_H
#define VIDEOTRACK_H
#include "videosource.h"
#include "safeobj.h"
#include "successivedifferenceestimator.h"
#include <livekit/rtc/stats/StatsListener.h>
#include <QObject>
#include <QQmlEngine>
#include <memory>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrack : public VideoSource, private LiveKitCpp::StatsListener
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VideoTrack)
private:
    class BitrateEstimator
    {
    public:
        void reset();
        void setBytes(uint64_t bytes, int64_t timeStampMSecsSinceEpoch);
        bool isValid() const;
        // output value, return current BPS
        uint64_t bitrate() const;
        explicit operator bool () const noexcept { return isValid(); }
    private:
        mutable QReadWriteLock _lock;
        SuccessiveDifferenceEstimator<uint64_t> _bytes;
        SuccessiveDifferenceEstimator<int64_t> _time;
    };
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
    bool isScreencast() const;
    bool isRemote() const { return _remote; }
    NetworkPriority networkPriority() const;
    ContentHint contentHint() const;
    DegradationPreference degradationPreference() const;
    // overrides of VideoSource
    bool isMuted() const final;
    QString stats() const final;
public slots:
    void setMuted(bool muted) override;
    void setNetworkPriority(NetworkPriority priority);
    void setContentHint(ContentHint hint);
    void setDegradationPreference(DegradationPreference preference);
    Q_INVOKABLE void queryStats();
signals:
    void networkPriorityChanged();
    void contentHintChanged();
    void degradationPreferenceChanged();
protected:
    // overrides of VideoSource
    bool metricsAllowed() const final;
    void subsribe(bool subscribe) final;
private:
    static QString codec(const LiveKitCpp::Stats& stats);
    static QString outboundStats(const LiveKitCpp::Stats& stats,
                                 const BitrateEstimator& bitrate,
                                 const QString& codec = {});
    static QString inboundStats(const LiveKitCpp::Stats& stats,
                                const BitrateEstimator& bitrate,
                                const QString& codec = {});
    static QString bitrateInfo(const BitrateEstimator& bitrate);
    static LiveKitCpp::Stats lookup(LiveKitCpp::StatsType type, const LiveKitCpp::StatsReport& report);
    void resetStats();
    void applyMute(bool mute);
    // impl. of LiveKitCpp::StatsListener
    void onStats(const LiveKitCpp::StatsReport& report) final;
    // impl. of LiveKitCpp::MediaEventsListener
    void onMuteChanged(const std::string&, bool muted) final { applyMute(muted); }
    void onRemoteSideMuteChanged(const std::string&, bool muted) final { applyMute(muted); }
private:
    const bool _remote = false;
    std::shared_ptr<LiveKitCpp::VideoTrack> _sdkTrack;
    SafeObj<LiveKitCpp::Stats> _rtpStats;
    SafeObj<LiveKitCpp::Stats> _codecStats;
    BitrateEstimator _bitrate;
};

#endif // VIDEOTRACK_H
