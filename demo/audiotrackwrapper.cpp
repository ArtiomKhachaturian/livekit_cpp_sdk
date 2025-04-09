#include "audiotrackwrapper.h"
#include <media/AudioTrack.h>

AudioTrackWrapper::AudioTrackWrapper(QObject* parent)
    : QObject(parent)
{
}

AudioTrackWrapper::AudioTrackWrapper(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl,
                                     QObject *parent)
    : QObject(parent)
    , _impl(impl)
{
    if (_impl) {
        _impl->addListener(this);
    }
}

AudioTrackWrapper::~AudioTrackWrapper()
{
    if (_impl) {
        _impl->removeListener(this);
    }
}

void AudioTrackWrapper::setVolume(qreal volume)
{
    if (_impl) {
        _impl->setVolume(volume);
    }
}

QString AudioTrackWrapper::id() const
{
    if (_impl) {
        return QString::fromStdString(_impl->id());
    }
    return {};
}

bool AudioTrackWrapper::muted() const
{
    return _impl && _impl->muted();
}

void AudioTrackWrapper::setMuted(bool mute)
{
    if (_impl) {
        _impl->mute(mute);
    }
}
