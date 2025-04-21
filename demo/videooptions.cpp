#include "videooptions.h"

VideoOptions::VideoOptions(qint32 width, qint32 height, qint32 maxFps)
{
    setResolution(width, height);
    setMaxFPS(maxFps);
}

VideoOptions::VideoOptions(const LiveKitCpp::VideoOptions& src)
    : _impl(src)
{
}

VideoOptions& VideoOptions::operator = (const LiveKitCpp::VideoOptions& src)
{
    _impl = src;
    return *this;
}

bool VideoOptions::operator == (const VideoOptions& other) const noexcept
{
    return this == &other || _impl == other._impl;
}

bool VideoOptions::operator == (const LiveKitCpp::VideoOptions& other) const noexcept
{
    return _impl == other;
}

bool VideoOptions::operator != (const VideoOptions& other) const noexcept
{
    return _impl != other._impl;
}

bool VideoOptions::operator != (const LiveKitCpp::VideoOptions& other) const noexcept
{
    return _impl != other;
}

void VideoOptions::setResolution(qint32 width, qint32 height) noexcept
{
    setWidth(width);
    setHeight(height);
}

void VideoOptions::setResolution(const QSize& resolution) noexcept
{
    setResolution(resolution.width(), resolution.height());
}

QString VideoOptions::toString() const
{
    return QString::fromStdString(LiveKitCpp::toString(_impl));
}

