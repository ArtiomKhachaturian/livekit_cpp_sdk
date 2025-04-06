#include "trackwrapper.h"
#include <media/Track.h>

TrackWrapper::TrackWrapper(const std::weak_ptr<LiveKitCpp::Track>& impl,
                           QObject *parent)
    : QObject{parent}
    , _impl(impl)
{
}

QString TrackWrapper::id() const
{
    if (const auto impl = _impl.lock()) {
        return QString::fromStdString(impl->id());
    }
    return {};
}

bool TrackWrapper::muted() const
{
    if (const auto impl = _impl.lock()) {
        return impl->muted();
    }
    return false;
}

void TrackWrapper::setMuted(bool mute)
{
    const auto impl = _impl.lock();
    if (impl && impl->muted() != mute) {
        impl->mute(mute);
        emit muteChanged();
    }
}
