#include "videotrack.h"
#include "videofilter.h"
#include <livekit/rtc/media/LocalVideoTrack.h>
#include <QLocale>

namespace
{

inline QString formatCodecStrings(const std::optional<std::string>& implementation,
                                  const QString& codec) {
    QStringList values;
    if (implementation.has_value() && !implementation->empty()) {
        values.append(QString::fromStdString(implementation.value()));
    }
    if (!codec.isEmpty()) {
        values.append(codec);
    }
    return values.join(QStringLiteral("/"));
}

inline auto toMSecsSinceEpoch(const std::chrono::time_point<std::chrono::system_clock>& ts) {
    return ts.time_since_epoch().count();
}

}

void VideoTrack::BitrateEstimator::reset()
{
    const QWriteLocker locker(&_lock);
    _bytes.reset();
    _time.reset();
}

void VideoTrack::BitrateEstimator::setBytes(uint64_t bytes, int64_t timeStampMSecsSinceEpoch)
{
    const QWriteLocker locker(&_lock);
    _bytes.setCurrent(bytes);
    _time.setCurrent(timeStampMSecsSinceEpoch);
}

bool VideoTrack::BitrateEstimator::isValid() const
{
    const QReadLocker locker(&_lock);
    return _time.hasEstimation();
}

uint64_t VideoTrack::BitrateEstimator::bitrate() const
{
    const QReadLocker locker(&_lock);
    if (const auto timeDiff = _time.diff()) { //diff in milliseconds
        return static_cast<uint64_t>(qRound((8 *_bytes.diff()) / (timeDiff / 1000.)));
    }
    return 0ULL;
}

VideoTrack::VideoTrack(QObject* parent)
    : VideoSource{parent}
{
}

VideoTrack::VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack,
                       QObject* parent)
    : VideoSource(parent)
    , _remote(sdkTrack && sdkTrack->remote())
    , _sdkTrack(sdkTrack)
{
    if (_sdkTrack) {
        _sdkTrack->addListener(this);
        _sdkTrack->addStatsListener(this);
    }
}

VideoTrack::~VideoTrack()
{
    if (_sdkTrack) {
        _sdkTrack->removeStatsListener(this);
        _sdkTrack->removeListener(this);
        _sdkTrack->removeSink(this);
    }
    if (const auto sdkTrack = localTrack()) {
        sdkTrack->setFilter(nullptr);
    }
}

QString VideoTrack::id() const
{
    if (_sdkTrack) {
        return QString::fromStdString(_sdkTrack->id());
    }
    return {};
}

bool VideoTrack::isScreencast() const
{
    return _sdkTrack && LiveKitCpp::TrackSource::ScreenShare == _sdkTrack->source();
}

bool VideoTrack::isSecure() const
{
    return _sdkTrack && LiveKitCpp::EncryptionType::None != _sdkTrack->encryption();
}

bool VideoTrack::isFirstPacketSent() const
{
    const auto sdkTrack = localTrack();
    return sdkTrack && sdkTrack->firstPacketSent();
}

VideoTrack::NetworkPriority VideoTrack::networkPriority() const
{
    if (const auto sdkTrack = localTrack()) {
        switch (sdkTrack->networkPriority()) {
            case LiveKitCpp::NetworkPriority::VeryLow:
                break;
            case LiveKitCpp::NetworkPriority::Low:
                return NetworkPriority::Low;
            case LiveKitCpp::NetworkPriority::Medium:
                return NetworkPriority::Medium;
            case LiveKitCpp::NetworkPriority::High:
                return NetworkPriority::High;
            default:
                Q_ASSERT(false);
                break;
        }
    }
    return NetworkPriority::VeryLow;
}

VideoTrack::ContentHint VideoTrack::contentHint() const
{
    if (_sdkTrack) {
        switch (_sdkTrack->contentHint()) {
            case LiveKitCpp::VideoContentHint::None:
                break;
            case LiveKitCpp::VideoContentHint::Motion:
                return ContentHint::Motion;
            case LiveKitCpp::VideoContentHint::Detailed:
                return ContentHint::Detailed;
            case LiveKitCpp::VideoContentHint::Text:
                return ContentHint::Text;
            default:
                Q_ASSERT(false);
                break;
        }
    }
    return ContentHint::None;
}

VideoTrack::DegradationPreference VideoTrack::degradationPreference() const
{
    if (const auto sdkTrack = localTrack()) {
        switch (sdkTrack->degradationPreference()) {
            case LiveKitCpp::DegradationPreference::Default:
                break;
            case LiveKitCpp::DegradationPreference::Disabled:
                return DegradationPreference::Disabled;
            case LiveKitCpp::DegradationPreference::MaintainFramerate:
                return DegradationPreference::MaintainFramerate;
            case LiveKitCpp::DegradationPreference::MaintainResolution:
                return DegradationPreference::MaintainResolution;
            case LiveKitCpp::DegradationPreference::Balanced:
                return DegradationPreference::Balanced;
            default:
                Q_ASSERT(false);
                break;
        }
    }
    return DegradationPreference::Default;
}

VideoTrack::ScalabilityMode VideoTrack::scalabilityMode() const
{
    if (const auto sdkTrack = localTrack()) {
        switch (sdkTrack->scalabilityMode()) {
            case LiveKitCpp::VideoScalabilityMode::Auto:
                break;
            case LiveKitCpp::VideoScalabilityMode::L1T1:
                return ScalabilityMode::L1T1;
            case LiveKitCpp::VideoScalabilityMode::L1T2:
                return ScalabilityMode::L1T2;
            case LiveKitCpp::VideoScalabilityMode::L1T3:
                return ScalabilityMode::L1T3;
            case LiveKitCpp::VideoScalabilityMode::L2T1:
                return ScalabilityMode::L2T1;
            case LiveKitCpp::VideoScalabilityMode::L2T2:
                return ScalabilityMode::L2T2;
            case LiveKitCpp::VideoScalabilityMode::L2T3:
                return ScalabilityMode::L2T3;
            case LiveKitCpp::VideoScalabilityMode::L3T1:
                return ScalabilityMode::L3T1;
            case LiveKitCpp::VideoScalabilityMode::L3T2:
                return ScalabilityMode::L3T2;
            case LiveKitCpp::VideoScalabilityMode::L3T3:
                return ScalabilityMode::L3T3;
            case LiveKitCpp::VideoScalabilityMode::L2T1h:
                return ScalabilityMode::L2T1h;
            case LiveKitCpp::VideoScalabilityMode::L2T2h:
                return ScalabilityMode::L2T2h;
            case LiveKitCpp::VideoScalabilityMode::L2T3h:
                return ScalabilityMode::L2T3h;
            case LiveKitCpp::VideoScalabilityMode::L3T1h:
                return ScalabilityMode::L3T1h;
            case LiveKitCpp::VideoScalabilityMode::L3T2h:
                return ScalabilityMode::L3T2h;
            case LiveKitCpp::VideoScalabilityMode::L3T3h:
                return ScalabilityMode::L3T3h;
            case LiveKitCpp::VideoScalabilityMode::S2T1:
                return ScalabilityMode::S2T1;
            case LiveKitCpp::VideoScalabilityMode::S2T2:
                return ScalabilityMode::S2T2;
            case LiveKitCpp::VideoScalabilityMode::S2T3:
                return ScalabilityMode::S2T3;
            case LiveKitCpp::VideoScalabilityMode::S2T1h:
                return ScalabilityMode::S2T1h;
            case LiveKitCpp::VideoScalabilityMode::S2T2h:
                return ScalabilityMode::S2T2h;
            case LiveKitCpp::VideoScalabilityMode::S2T3h:
                return ScalabilityMode::S2T3h;
            case LiveKitCpp::VideoScalabilityMode::S3T1:
                return ScalabilityMode::S3T1;
            case LiveKitCpp::VideoScalabilityMode::S3T2:
                return ScalabilityMode::S3T2;
            case LiveKitCpp::VideoScalabilityMode::S3T3:
                return ScalabilityMode::S3T3;
            case LiveKitCpp::VideoScalabilityMode::S3T1h:
                return ScalabilityMode::S3T1h;
            case LiveKitCpp::VideoScalabilityMode::S3T2h:
                return ScalabilityMode::S3T2h;
            case LiveKitCpp::VideoScalabilityMode::S3T3h:
                return ScalabilityMode::S3T3h;
            case LiveKitCpp::VideoScalabilityMode::L2T2Key:
                return ScalabilityMode::L2T2Key;
            case LiveKitCpp::VideoScalabilityMode::L2T2KeyShift:
                return ScalabilityMode::L2T2KeyShift;
            case LiveKitCpp::VideoScalabilityMode::L2T3Key:
                return ScalabilityMode::L2T3Key;
            case LiveKitCpp::VideoScalabilityMode::L2T3KeyShift:
                return ScalabilityMode::L2T3KeyShift;
            case LiveKitCpp::VideoScalabilityMode::L3T1Key:
                return ScalabilityMode::L3T1Key;
            case LiveKitCpp::VideoScalabilityMode::L3T2Key:
                return ScalabilityMode::L3T2Key;
            case LiveKitCpp::VideoScalabilityMode::L3T2KeyShift:
                return ScalabilityMode::L3T2KeyShift;
            case LiveKitCpp::VideoScalabilityMode::L3T3Key:
                return ScalabilityMode::L3T3Key;
            case LiveKitCpp::VideoScalabilityMode::L3T3KeyShift:
                return ScalabilityMode::L3T3KeyShift;
            default:
                Q_ASSERT(false);
                break;
        }
    }
    return ScalabilityMode::Auto;
}

MediaDeviceInfo VideoTrack::deviceInfo() const
{
    if (const auto sdkTrack = localTrack()) {
        return sdkTrack->deviceInfo();
    }
    return {};
}

VideoOptions VideoTrack::options() const
{
    if (const auto sdkTrack = localTrack()) {
        return sdkTrack->options();
    }
    return {};
}

void VideoTrack::setDeviceInfo(const MediaDeviceInfo& info)
{
    if (const auto sdkTrack = localTrack()) {
        sdkTrack->setDeviceInfo(info);
    }
}

void VideoTrack::setOptions(const VideoOptions& options)
{
    if (const auto sdkTrack = localTrack()) {
        sdkTrack->setOptions(options);
    }
}

void VideoTrack::queryStats()
{
    if (_sdkTrack && !isMuted()) {
        _sdkTrack->queryStats();
    }
}

bool VideoTrack::isMuted() const
{
    if (_sdkTrack) {
        if (_sdkTrack->muted()) {
            return true;
        }
        // ignore notifications from remote participants that
        // my video stream was disabled on their side
        // we accept only of their stream changes
        if (_sdkTrack->remote() && _sdkTrack->remoteMuted()) {
            return true;
        }
    }
    return false;
}

QString VideoTrack::stats() const
{
    if (_sdkTrack) {
        QString report;
        if (isRemote()) {
            report = inboundStats(_rtpStats, _bitrate, codec(_codecStats));
        }
        else {
            report = outboundStats(_rtpStats, _bitrate, codec(_codecStats));
        }
        if (!report.isEmpty()) {
            return report;
        }
    }
    return VideoSource::stats();
}

QString VideoTrack::name() const
{
    if (const auto sdkTrack = localTrack()) {
        return QString::fromStdString(sdkTrack->deviceInfo()._name);
    }
    return VideoSource::name();
}

void VideoTrack::setMuted(bool mute)
{
    if (_sdkTrack) {
        _sdkTrack->mute(mute);
    }
}

void VideoTrack::setNetworkPriority(NetworkPriority priority)
{
    if (const auto sdkTrack = localTrack()) {
        LiveKitCpp::NetworkPriority sdkPriority = LiveKitCpp::NetworkPriority::VeryLow;
        switch (priority) {
            case NetworkPriority::VeryLow:
                break;
            case NetworkPriority::Low:
                sdkPriority = LiveKitCpp::NetworkPriority::Low;
                break;
            case NetworkPriority::Medium:
                sdkPriority = LiveKitCpp::NetworkPriority::Medium;
                break;
            case NetworkPriority::High:
                sdkPriority = LiveKitCpp::NetworkPriority::High;
                break;
            default:
                Q_ASSERT(false);
                break;
        }
        if (sdkPriority != sdkTrack->networkPriority()) {
            sdkTrack->setNetworkPriority(sdkPriority);
            if (sdkPriority == sdkTrack->networkPriority()) {
                emit networkPriorityChanged();
            }
        }
    }
}

void VideoTrack::setContentHint(ContentHint hint)
{
    if (_sdkTrack) {
        LiveKitCpp::VideoContentHint sdkHint = LiveKitCpp::VideoContentHint::None;
        switch (hint) {
            case None:
                break;
            case Motion:
                sdkHint = LiveKitCpp::VideoContentHint::Motion;
                break;
            case Detailed:
                sdkHint = LiveKitCpp::VideoContentHint::Detailed;
                break;
            case Text:
                sdkHint = LiveKitCpp::VideoContentHint::Text;
                break;
            default:
                Q_ASSERT(false);
                break;
        }
        if (sdkHint != _sdkTrack->contentHint()) {
            _sdkTrack->setContentHint(sdkHint);
            if (sdkHint == _sdkTrack->contentHint()) {
                emit contentHintChanged();
            }
        }
    }
}

void VideoTrack::setDegradationPreference(DegradationPreference preference)
{
    if (const auto sdkTrack = localTrack()) {
        LiveKitCpp::DegradationPreference sdkPreference = LiveKitCpp::DegradationPreference::Default;
        switch (preference) {
            case DegradationPreference::Default:
                break;
            case Disabled:
                sdkPreference = LiveKitCpp::DegradationPreference::Disabled;
                break;
            case MaintainFramerate:
                sdkPreference = LiveKitCpp::DegradationPreference::MaintainFramerate;
                break;
            case MaintainResolution:
                sdkPreference = LiveKitCpp::DegradationPreference::MaintainResolution;
                break;
            case Balanced:
                sdkPreference = LiveKitCpp::DegradationPreference::Balanced;
                break;
            default:
                Q_ASSERT(false);
                break;
        }
        if (sdkPreference != sdkTrack->degradationPreference()) {
            sdkTrack->setDegradationPreference(sdkPreference);
            if (sdkPreference == sdkTrack->degradationPreference()) {
                emit degradationPreferenceChanged();
            }
        }
    }
}

void VideoTrack::setScalabilityMode(ScalabilityMode mode)
{
    if (const auto sdkTrack = localTrack()) {
        LiveKitCpp::VideoScalabilityMode sdkMode = LiveKitCpp::VideoScalabilityMode::Auto;
        switch (mode) {
            case ScalabilityMode::Auto:
                break;
            case ScalabilityMode::L1T1:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L1T1;
                break;
            case ScalabilityMode::L1T2:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L1T2;
                break;
            case ScalabilityMode::L1T3:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L1T3;
                break;
            case ScalabilityMode::L2T1:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T1;
                break;
            case ScalabilityMode::L2T2:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T2;
                break;
            case ScalabilityMode::L2T3:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T3;
                break;
            case ScalabilityMode::L3T1:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T1;
                break;
            case ScalabilityMode::L3T2:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T2;
                break;
            case ScalabilityMode::L3T3:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T3;
                break;
            case ScalabilityMode::L2T1h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T1h;
                break;
            case ScalabilityMode::L2T2h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T2h;
                break;
            case ScalabilityMode::L2T3h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T3h;
                break;
            case ScalabilityMode::L3T1h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T1h;
                break;
            case ScalabilityMode::L3T2h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T2h;
                break;
            case ScalabilityMode::L3T3h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T3h;
                break;
            case ScalabilityMode::S2T1:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S2T1;
                break;
            case ScalabilityMode::S2T2:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S2T2;
                break;
            case ScalabilityMode::S2T3:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S2T3;
                break;
            case ScalabilityMode::S2T1h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S2T1h;
                break;
            case ScalabilityMode::S2T2h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S2T2h;
                break;
            case ScalabilityMode::S2T3h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S2T3h;
                break;
            case ScalabilityMode::S3T1:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S3T1;
                break;
            case ScalabilityMode::S3T2:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S3T2;
                break;
            case ScalabilityMode::S3T3:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S3T3;
                break;
            case ScalabilityMode::S3T1h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S3T1h;
                break;
            case ScalabilityMode::S3T2h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S3T2h;
                break;
            case ScalabilityMode::S3T3h:
                sdkMode = LiveKitCpp::VideoScalabilityMode::S3T3h;
                break;
            case ScalabilityMode::L2T2Key:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T2Key;
                break;
            case ScalabilityMode::L2T2KeyShift:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T2KeyShift;
                break;
            case ScalabilityMode::L2T3Key:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T3Key;
                break;
            case ScalabilityMode::L2T3KeyShift:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L2T3KeyShift;
                break;
            case ScalabilityMode::L3T1Key:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T1Key;
                break;
            case ScalabilityMode::L3T2Key:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T2Key;
                break;
            case ScalabilityMode::L3T2KeyShift:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T2KeyShift;
                break;
            case ScalabilityMode::L3T3Key:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T3Key;
                break;
            case ScalabilityMode::L3T3KeyShift:
                sdkMode = LiveKitCpp::VideoScalabilityMode::L3T3KeyShift;
                break;
            default:
                Q_ASSERT(false);
                break;
        }
        if (sdkMode != sdkTrack->scalabilityMode()) {
            sdkTrack->setScalabilityMode(sdkMode);
            if (sdkMode == sdkTrack->scalabilityMode()) {
                emit scalabilityModeChanged();
            }
        }
    }
}

void VideoTrack::applyFilter(VideoFilter* filter)
{
    VideoSource::applyFilter(filter);
    if (const auto sdkTrack = localTrack()) {
        sdkTrack->setFilter(filter);
    }
}

bool VideoTrack::metricsAllowed() const
{
    return !_rtpStats.get();
}

void VideoTrack::subsribe(bool subscribe)
{
    if (_sdkTrack) {
        if (subscribe) {
            _sdkTrack->addSink(this);
        }
        else {
            _sdkTrack->removeSink(this);
        }
        resetStats();
    }
}

QString VideoTrack::codec(const LiveKitCpp::Stats& stats)
{
    const auto codec = stats.extCodec();
    if (codec && codec->mimeType().has_value()) {
        const auto mime = QString::fromStdString(codec->mimeType().value());
        const auto mimeSplitted = mime.split("/");
        if (2 == mimeSplitted.size()) {
            return mimeSplitted.back();
        }
        return mime;
    }
    return {};
}

QString VideoTrack::outboundStats(const LiveKitCpp::Stats& stats,
                                  const BitrateEstimator& bitrate,
                                  const QString& codec) const
{
    if (const auto rtp = stats.extOutboundRtpStream()) {
        QStringList parameters;
        if (rtp->frameWidth().has_value() && rtp->frameHeight().has_value()) {
            parameters.append(formatVideoInfo(rtp->frameWidth().value(),
                                              rtp->frameHeight().value(),
                                              static_cast<quint16>(rtp->framesPerSecond().value_or(0.))));
            parameters.append(bitrateInfo(bitrate));
        }
        const auto encoder = formatCodecStrings(rtp->encoderImplementation(), codec);
        if (!encoder.isEmpty()) {
            parameters.append(tr("encoder: %1").arg(encoder));
        }
        if (const auto frames = rtp->framesEncoded().value_or(0)) {
            parameters.append(tr("encoded frames: %1").arg(frames));
        }
        if (const auto frames = rtp->framesSent().value_or(0)) {
            parameters.append(tr("sent frames: %1").arg(frames));
        }
        if (rtp->scalabilityMode().has_value()) {
            const auto scalability = QString::fromStdString(rtp->scalabilityMode().value());
            parameters.append(tr("scalability: %1").arg(scalability));
        }
        return parameters.join(QStringLiteral("\n"));
    }
    return {};
}

QString VideoTrack::inboundStats(const LiveKitCpp::Stats& stats,
                                 const BitrateEstimator& bitrate,
                                 const QString& codec) const
{
    if (const auto rtp = stats.extInboundRtpStream()) {
        QStringList parameters;
        if (rtp->frameWidth().has_value() && rtp->frameHeight().has_value()) {
            parameters.append(formatVideoInfo(rtp->frameWidth().value(),
                                              rtp->frameHeight().value(),
                                              static_cast<quint16>(rtp->framesPerSecond().value_or(0.))));
            parameters.append(bitrateInfo(bitrate));
        }
        const auto decoder = formatCodecStrings(rtp->decoderImplementation(), codec);
        if (!decoder.isEmpty()) {
            parameters.append(tr("decoder: %1").arg(decoder));
        }
        if (const auto frames = rtp->framesReceived().value_or(0)) {
            parameters.append(tr("received frames: %1").arg(frames));
        }
        if (const auto frames = rtp->framesDecoded().value_or(0)) {
            parameters.append(tr("decoded frames: %1").arg(frames));
        }
        if (const auto frames = rtp->framesDropped().value_or(0)) {
            parameters.append(tr("dropped frames: %1").arg(frames));
        }
        if (rtp->contentType().has_value()) {
            const auto contentType = QString::fromStdString(rtp->contentType().value());
            parameters.append(tr("content: %1").arg(contentType));
        }
        return parameters.join(QStringLiteral("\n"));
    }
    return {};
}

QString VideoTrack::bitrateInfo(const BitrateEstimator& bitrate)
{
    QLocale loc;
    return tr("bitrate: %1").arg(loc.formattedDataSize(bitrate.bitrate()));
}

LiveKitCpp::Stats VideoTrack::lookup(LiveKitCpp::StatsType type,
                                     const LiveKitCpp::StatsReport& report)
{
    for (auto stats : report) {
        if (type == stats.type()) {
            return stats;
        }
    }
    return {};
}

void VideoTrack::resetStats()
{
    _bitrate.reset();
    _rtpStats = LiveKitCpp::Stats{};
    _codecStats = LiveKitCpp::Stats{};
}

void VideoTrack::applyMute(bool mute)
{
    if (mute) {
        resetStats();
        stopMetricsCollection();
        emit statsChanged();
    }
    emit muteChanged();
}

std::shared_ptr<const LiveKitCpp::LocalVideoTrack> VideoTrack::localTrack() const
{
    return std::dynamic_pointer_cast<const LiveKitCpp::LocalVideoTrack>(_sdkTrack);
}

std::shared_ptr<LiveKitCpp::LocalVideoTrack> VideoTrack::localTrack()
{
    return std::dynamic_pointer_cast<LiveKitCpp::LocalVideoTrack>(_sdkTrack);
}

void VideoTrack::onStats(const LiveKitCpp::StatsReport& report)
{
    if (_remote) {
        const auto stats = lookup(LiveKitCpp::StatsType::InboundRtp, report);
        if (const auto rtp = stats.extInboundRtpStream()) {
            _bitrate.setBytes(rtp->bytesReceived().value_or(0), toMSecsSinceEpoch(report.timestamp()));
            stopMetricsCollection();
        }
        _rtpStats = stats;
    }
    else {
        const auto stats = lookup(LiveKitCpp::StatsType::OutboundRtp, report);
        if (const auto rtp = stats.extOutboundRtpStream()) {
            _bitrate.setBytes(rtp->bytesSent().value_or(0), toMSecsSinceEpoch(report.timestamp()));
            stopMetricsCollection();
        }
        _rtpStats = stats;
    }
    _codecStats = lookup(LiveKitCpp::StatsType::Codec, report);
    emit statsChanged();
}

void VideoTrack::onMediaChanged(const std::string&)
{
    emit deviceInfoChanged();
}

void VideoTrack::onMediaOptionsChanged(const std::string&)
{
    emit optionsChanged();
}

