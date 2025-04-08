#include "audiotrackwrapper.h"
#include <media/AudioTrack.h>

AudioTrackWrapper::AudioTrackWrapper(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl,
                                     QObject *parent)
    : QObject(parent)
    , _impl(impl)
{
    if (impl) {
        impl->addListener(this);
    }
}

AudioTrackWrapper::~AudioTrackWrapper()
{
    if (const auto impl = _impl.lock()) {
        impl->removeListener(this);
    }
}

void AudioTrackWrapper::setVolume(qreal volume)
{
    if (const auto impl = _impl.lock()) {
        impl->setVolume(volume);
    }
}

QString AudioTrackWrapper::id() const
{
    if (const auto impl = _impl.lock()) {
        return QString::fromStdString(impl->id());
    }
    return {};
}

bool AudioTrackWrapper::muted() const
{
    if (const auto impl = _impl.lock()) {
        return impl->muted();
    }
    return false;
}

void AudioTrackWrapper::setMuted(bool mute)
{
    const auto impl = _impl.lock();
    if (impl && impl->muted() != mute) {
        impl->mute(mute);
    }
}
