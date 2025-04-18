#ifndef VIDEOOPTIONS_H
#define VIDEOOPTIONS_H
#include <livekit/rtc/media/VideoOptions.h>
#include <QCoreApplication>
#include <QSize>
#include <QtQml/qqmlregistration.h>

class VideoOptions
{
    QML_VALUE_TYPE(videoOptions)
    Q_DECLARE_TR_FUNCTIONS(VideoOptions)
public:
    VideoOptions() = default;
    VideoOptions(const VideoOptions&) = default;
    VideoOptions(VideoOptions&&) noexcept = default;
    VideoOptions(qint32 width, qint32 height, qint32 maxFps);
    VideoOptions(const LiveKitCpp::VideoOptions& src);
    virtual ~VideoOptions() = default;
    VideoOptions& operator = (const VideoOptions&) = default;
    VideoOptions& operator = (VideoOptions&&) noexcept = default;
    VideoOptions& operator = (const LiveKitCpp::VideoOptions& src);
    bool operator == (const VideoOptions& other) const noexcept;
    bool operator == (const LiveKitCpp::VideoOptions& other) const noexcept;
    bool operator != (const VideoOptions& other) const noexcept;
    bool operator != (const LiveKitCpp::VideoOptions& other) const noexcept;
    operator LiveKitCpp::VideoOptions() const { return _impl; }
    operator QString() const { return toString(); }
    explicit operator bool () const noexcept { return isValid(); }
    qint32 width() const noexcept { return _impl._width; }
    qint32 height() const noexcept { return _impl._height; }
    qint32 maxFPS() const noexcept { return _impl._maxFPS; }
    void setWidth(qint32 width) noexcept { _impl._width = width; }
    void setHeight(qint32 height) noexcept { _impl._height = height; }
    void setMaxFPS(qint32 maxFps) noexcept { _impl._maxFPS = maxFps; }
    void setResolution(qint32 width, qint32 height) noexcept;
    void setResolution(const QSize& resolution) noexcept;
    QSize resolution() const noexcept { return {width(), height()}; }
    QString toString() const;
    bool isValid() const noexcept { return !_impl.null(); }
private:
    LiveKitCpp::VideoOptions _impl;
};

#endif // VIDEOOPTIONS_H
