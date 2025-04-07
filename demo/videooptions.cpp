#include "videooptions.h"

VideoOptions::VideoOptions(qint32 width, qint32 height, qint32 maxFps)
{
    setResolution(width, height);
    setMaxFPS(maxFps);
}

VideoOptions::VideoOptions(const LiveKitCpp::VideoOptions& src)
    : VideoOptions(src._width, src._height, src._maxFPS)
{
}

VideoOptions& VideoOptions::operator = (const LiveKitCpp::VideoOptions& src)
{
    setResolution(src._width, src._height);
    setMaxFPS(src._maxFPS);
    return *this;
}

bool VideoOptions::operator == (const VideoOptions& other) const noexcept
{
    return this == &other || (width() == other.width() && height() == other.height() && maxFPS() == other.maxFPS());
}

bool VideoOptions::operator == (const LiveKitCpp::VideoOptions& other) const noexcept
{
    return width() == other._width && height() == other._height && maxFPS() == other._maxFPS;
}

bool VideoOptions::operator != (const VideoOptions& other) const noexcept
{
    if (this != &other) {
        return width() != other.width() || height() != other.height() || maxFPS() != other.maxFPS();
    }
    return false;
}

bool VideoOptions::operator != (const LiveKitCpp::VideoOptions& other) const noexcept
{
    return width() != other._width || height() != other._height || maxFPS() != other._maxFPS;
}

VideoOptions::operator LiveKitCpp::VideoOptions() const
{
    return {width(), height(), maxFPS()};
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
    return tr("%1x%2 %3 fps").arg(width()).arg(height()).arg(maxFPS());
}

