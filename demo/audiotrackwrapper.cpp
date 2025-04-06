#include "audiotrackwrapper.h"
#include <media/AudioTrack.h>

AudioTrackWrapper::AudioTrackWrapper(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl,
                                     QObject *parent)
    : TrackWrapper(impl, parent)
    , _impl(impl)
{
}

void AudioTrackWrapper::setVolume(qreal volume)
{
    if (_impl) {
        _impl->setVolume(volume);
    }
}
