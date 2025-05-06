#include "videotrack.h"
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
    takeSdkTrack();
}

std::shared_ptr<LiveKitCpp::VideoTrack> VideoTrack::takeSdkTrack()
{
    if (_sdkTrack) {
        _sdkTrack->removeStatsListener(this);
        _sdkTrack->removeListener(this);
        _sdkTrack->removeSink(this);
        return std::move(_sdkTrack);
    }
    return nullptr;
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

VideoTrack::NetworkPriority VideoTrack::networkPriority() const
{
    if (const auto localSdkTrack = std::dynamic_pointer_cast<const LiveKitCpp::LocalVideoTrack>(_sdkTrack)) {
        switch (localSdkTrack->networkPriority()) {
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
            case LiveKitCpp::VideoContentHint::Fluid:
                return ContentHint::Fluid;
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
    if (const auto localSdkTrack = std::dynamic_pointer_cast<const LiveKitCpp::LocalVideoTrack>(_sdkTrack)) {
        switch (localSdkTrack->degradationPreference()) {
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

void VideoTrack::setMuted(bool mute)
{
    if (_sdkTrack) {
        _sdkTrack->mute(mute);
    }
}

void VideoTrack::setNetworkPriority(NetworkPriority priority)
{
    if (const auto localSdkTrack = std::dynamic_pointer_cast<LiveKitCpp::LocalVideoTrack>(_sdkTrack)) {
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
        if (sdkPriority != localSdkTrack->networkPriority()) {
            localSdkTrack->setNetworkPriority(sdkPriority);
            emit networkPriorityChanged();
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
            case Fluid:
                sdkHint = LiveKitCpp::VideoContentHint::Fluid;
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
            emit contentHintChanged();
        }
    }
}

void VideoTrack::setDegradationPreference(DegradationPreference preference)
{
    if (const auto localSdkTrack = std::dynamic_pointer_cast<LiveKitCpp::LocalVideoTrack>(_sdkTrack)) {
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
        if (sdkPreference != localSdkTrack->degradationPreference()) {
            localSdkTrack->setDegradationPreference(sdkPreference);
            emit degradationPreferenceChanged();
        }
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

