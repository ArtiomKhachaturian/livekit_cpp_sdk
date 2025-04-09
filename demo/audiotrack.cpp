#include "audiotrack.h"
#include <livekit/media/AudioTrack.h>

AudioTrack::AudioTrack(QObject* parent)
    : QObject(parent)
{
}

AudioTrack::AudioTrack(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl,
                                     QObject *parent)
    : QObject(parent)
    , _impl(impl)
{
    if (_impl) {
        _impl->addListener(this);
    }
}

AudioTrack::~AudioTrack()
{
    if (_impl) {
        _impl->removeListener(this);
    }
}

void AudioTrack::setVolume(qreal volume)
{
    if (_impl) {
        _impl->setVolume(volume);
    }
}

QString AudioTrack::id() const
{
    if (_impl) {
        return QString::fromStdString(_impl->id());
    }
    return {};
}

bool AudioTrack::muted() const
{
    return _impl && _impl->muted();
}

void AudioTrack::setMuted(bool mute)
{
    if (_impl) {
        _impl->mute(mute);
    }
}
