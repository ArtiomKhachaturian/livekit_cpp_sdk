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
    // https://www.w3.org/TR/webrtc-svc/#scalabilitymodes*
    enum ScalabilityMode
    {
        Auto,
        L1T1,
        L1T2,
        L1T3,
        L2T1,
        L2T2,
        L2T3,
        L3T1,
        L3T2,
        L3T3,
        L2T1h,
        L2T2h,
        L2T3h,
        L3T1h,
        L3T2h,
        L3T3h,
        S2T1,
        S2T2,
        S2T3,
        S2T1h,
        S2T2h,
        S2T3h,
        S3T1,
        S3T2,
        S3T3,
        S3T1h,
        S3T2h,
        S3T3h,
        L2T2Key,
        L2T2KeyShift,
        L2T3Key,
        L2T3KeyShift,
        L3T1Key,
        L3T2Key,
        L3T2KeyShift,
        L3T3Key,
        L3T3KeyShift,
    };

public:
    Q_ENUM(NetworkPriority)
    Q_ENUM(ContentHint)
    Q_ENUM(DegradationPreference)
    Q_ENUM(ScalabilityMode)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool screencast READ isScreencast CONSTANT)
    Q_PROPERTY(bool remote READ isRemote CONSTANT)
    Q_PROPERTY(NetworkPriority networkPriority READ networkPriority WRITE setNetworkPriority NOTIFY networkPriorityChanged)
    Q_PROPERTY(ContentHint contentHint READ contentHint WRITE setContentHint NOTIFY contentHintChanged)
    Q_PROPERTY(DegradationPreference degradationPreference READ degradationPreference WRITE setDegradationPreference NOTIFY degradationPreferenceChanged)
    Q_PROPERTY(ScalabilityMode scalabilityMode READ scalabilityMode WRITE setScalabilityMode NOTIFY scalabilityModeChanged)
    Q_PROPERTY(bool secure READ isSecure CONSTANT)
public:
    explicit VideoTrack(QObject* parent = nullptr);
    VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack,
               QObject* parent = nullptr);
    ~VideoTrack() override;
    std::shared_ptr<LiveKitCpp::VideoTrack> takeSdkTrack();
    QString id() const;
    bool isScreencast() const;
    bool isRemote() const { return _remote; }
    bool isSecure() const;
    NetworkPriority networkPriority() const;
    ContentHint contentHint() const;
    DegradationPreference degradationPreference() const;
    ScalabilityMode scalabilityMode() const;
    // overrides of VideoSource
    bool isMuted() const final;
    QString stats() const final;
public slots:
    void setMuted(bool muted) override;
    void setNetworkPriority(NetworkPriority priority);
    void setContentHint(ContentHint hint);
    void setDegradationPreference(DegradationPreference preference);
    void setScalabilityMode(ScalabilityMode mode);
    Q_INVOKABLE void queryStats();
signals:
    void networkPriorityChanged();
    void contentHintChanged();
    void degradationPreferenceChanged();
    void scalabilityModeChanged();
protected:
    // overrides of VideoSource
    bool metricsAllowed() const final;
    void subsribe(bool subscribe) final;
private:
    static QString codec(const LiveKitCpp::Stats& stats);
    QString outboundStats(const LiveKitCpp::Stats& stats,
                          const BitrateEstimator& bitrate,
                          const QString& codec = {}) const;
    QString inboundStats(const LiveKitCpp::Stats& stats,
                        const BitrateEstimator& bitrate,
                        const QString& codec = {}) const;
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
