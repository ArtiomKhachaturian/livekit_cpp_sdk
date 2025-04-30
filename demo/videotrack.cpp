#include "videotrack.h"
#include <livekit/rtc/media/LocalVideoTrack.h>

VideoTrack::VideoTrack(QObject* parent)
    : VideoSource{parent}
{
}

VideoTrack::VideoTrack(const std::shared_ptr<LiveKitCpp::VideoTrack>& sdkTrack,
                       QObject* parent)
    : VideoSource(parent)
    , _sdkTrack(sdkTrack)
{
    if (_sdkTrack) {
        _sdkTrack->addListener(this);
    }
}

VideoTrack::~VideoTrack()
{
    takeSdkTrack();
}

std::shared_ptr<LiveKitCpp::VideoTrack> VideoTrack::takeSdkTrack()
{
    if (_sdkTrack) {
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

bool VideoTrack::muted() const
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

bool VideoTrack::isScreencast() const
{
    return _sdkTrack && LiveKitCpp::TrackSource::ScreenShare == _sdkTrack->source();
}

bool VideoTrack::isRemote() const
{
    return _sdkTrack && _sdkTrack->remote();
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

void VideoTrack::subsribe(bool subscribe)
{
    if (_sdkTrack) {
        if (subscribe) {
            _sdkTrack->addSink(this);
        }
        else {
            _sdkTrack->removeSink(this);
        }
    }
}

void VideoTrack::onMuteChanged(const std::string&, bool muted)
{
    if (muted) {
        stopMetricsCollection();
    }
    else {
        startMetricsCollection();
    }
    emit muteChanged();
}

void VideoTrack::onRemoteSideMuteChanged(const std::string&, bool)
{
    emit muteChanged();
}

