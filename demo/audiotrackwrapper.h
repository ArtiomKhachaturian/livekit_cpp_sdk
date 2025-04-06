#ifndef AUDIOTRACKWRAPPER_H
#define AUDIOTRACKWRAPPER_H
#include "trackwrapper.h"

namespace LiveKitCpp {
class AudioTrack;
}

class AudioTrackWrapper : public TrackWrapper
{
    Q_OBJECT
    QML_ELEMENT
public:
    AudioTrackWrapper(const std::shared_ptr<LiveKitCpp::AudioTrack>& impl = {},
                      QObject *parent = nullptr);
    const auto& track() const noexcept { return _impl; }
public slots:
    void setVolume(qreal volume);
private:
    const std::shared_ptr<LiveKitCpp::AudioTrack> _impl;
};

Q_DECLARE_METATYPE(AudioTrackWrapper*)

#endif // AUDIOTRACKWRAPPER_H
