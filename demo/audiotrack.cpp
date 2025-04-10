#include "audiotrack.h"
#include <livekit/media/AudioTrack.h>

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
    takeSdkTrack();
}

std::shared_ptr<LiveKitCpp::AudioTrack> AudioTrack::takeSdkTrack()
{
    if (_sdkTrack) {
        _sdkTrack->removeListener(this);
        return std::move(_sdkTrack);
    }
    return {};
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

bool AudioTrack::muted() const
{
    return _sdkTrack && _sdkTrack->muted();
}

void AudioTrack::setMuted(bool mute)
{
    if (_sdkTrack) {
        _sdkTrack->mute(mute);
    }
}
