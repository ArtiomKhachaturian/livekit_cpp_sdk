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
    if (sdkTrack) {
        sdkTrack->addListener(this);
    }
}

AudioTrack::~AudioTrack()
{
    takeSdkTrack();
}

std::shared_ptr<LiveKitCpp::AudioTrack> AudioTrack::takeSdkTrack()
{
    if (auto sdkTrack = _sdkTrack.take()) {
        sdkTrack->removeListener(this);
        return sdkTrack;
    }
    return {};
}

void AudioTrack::setVolume(qreal volume)
{
    if (const auto sdkTrack = _sdkTrack.get()) {
        sdkTrack->setVolume(volume);
    }
}

QString AudioTrack::id() const
{
    if (const auto sdkTrack = _sdkTrack.get()) {
        return QString::fromStdString(sdkTrack->id());
    }
    return {};
}

bool AudioTrack::muted() const
{
    const auto sdkTrack = _sdkTrack.get();
    return sdkTrack && sdkTrack->muted();
}

void AudioTrack::setMuted(bool mute)
{
    if (const auto sdkTrack = _sdkTrack.get()) {
        sdkTrack->mute(mute);
    }
}
