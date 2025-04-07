#ifndef AUDIOTRACKWRAPPER_H
#define AUDIOTRACKWRAPPER_H
#include "trackwrapper.h"

namespace LiveKitCpp {
class AudioTrack;
}

class AudioTrackWrapper : public TrackWrapper
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AudioTrackWrapper)
public:
    AudioTrackWrapper(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl = {},
                      QObject *parent = nullptr);
    std::shared_ptr<LiveKitCpp::AudioTrack> track() const noexcept { return _impl.lock(); }
public slots:
    void setVolume(qreal volume);
private:
    const std::weak_ptr<LiveKitCpp::AudioTrack> _impl;
};

Q_DECLARE_METATYPE(AudioTrackWrapper*)

#endif // AUDIOTRACKWRAPPER_H
