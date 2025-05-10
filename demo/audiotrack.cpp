#include "audiotrack.h"
#include <livekit/rtc/media/AudioTrack.h>

AudioTrack::AudioTrack(QObject* parent)
    : QObject(parent)
{
}

AudioTrack::AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& sdkTrack,
                       QObject *parent)
    : QObject(parent)
    , _sdkTrack(sdkTrack)
{
    if (_sdkTrack) {
        _sdkTrack->addListener(this);
    }
}

AudioTrack::~AudioTrack()
{
    if (_sdkTrack) {
        _sdkTrack->removeListener(this);
    }
}

void AudioTrack::setVolume(qreal volume)
{
    if (_sdkTrack) {
        _sdkTrack->setVolume(volume);
    }
}

QString AudioTrack::id() const
{
    if (_sdkTrack) {
        return QString::fromStdString(_sdkTrack->id());
    }
    return {};
}

bool AudioTrack::isMuted() const
{
    return _sdkTrack && _sdkTrack->muted();
}

void AudioTrack::setMuted(bool mute)
{
    if (_sdkTrack) {
        _sdkTrack->mute(mute);
    }
}
