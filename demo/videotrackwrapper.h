#ifndef VIDEOTRACKWRAPPER_H
#define VIDEOTRACKWRAPPER_H
#include "trackwrapper.h"
#include "safeobj.h"
#include <media/VideoTrackSink.h>
#include <QPointer>
#include <QVideoSink>

namespace LiveKitCpp {
class VideoTrack;
}

class VideoTrackWrapper : public TrackWrapper, private LiveKitCpp::VideoTrackSink
{
    Q_OBJECT
    QML_ELEMENT
public:
    VideoTrackWrapper(const std::shared_ptr<LiveKitCpp::VideoTrack>& impl = {},
                      QObject *parent = nullptr);
    ~VideoTrackWrapper() override;
    const auto& track() const noexcept { return _impl; }
public slots:
    void setVideoOutput(QVideoSink* output);
private:
    void onFrame(const std::shared_ptr<LiveKitCpp::VideoFrame>& frame) final;
private:
    const std::shared_ptr<LiveKitCpp::VideoTrack> _impl;
    SafeObj<QPointer<QVideoSink>> _output;
};

Q_DECLARE_METATYPE(VideoTrackWrapper*)

#endif // VIDEOTRACKWRAPPER_H
